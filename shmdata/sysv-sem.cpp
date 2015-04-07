/*
 * Copyright (C) 2015 Nicolas Bouillot (http://www.nicolasbouillot.net)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>  // perror
#include <iostream>
#include <thread>  //yield
#include "./sysv-sem.hpp"


namespace shmdata{

namespace semops{
// sem_num 0 is for reading, 1 is for writer, 2 is for data available, 3 for going to read
static struct sembuf sem_init [] = {{2, 1, SEM_UNDO}};
static struct sembuf read_wait [] = {{2, 0, SEM_UNDO},       // wait data
                                     {3, 1, SEM_UNDO}};      // incr going to read
static struct sembuf read_start [] = {{0, 1, SEM_UNDO},      // incr reader
                                      {1, 0, SEM_UNDO},      // wait writer
                                      {3, -1, SEM_UNDO}};    // decr going to read
static struct sembuf read_end [] = {{0, -1, SEM_UNDO}};      // decr reader
static struct sembuf write_start [] = {{0, 0, SEM_UNDO},     // wait reader is 0
                                        {1, 1, SEM_UNDO},    // incr writer
                                       {0, 1, SEM_UNDO},    // incr reader
                                        {2, -1, SEM_UNDO}};  // updating data
static struct sembuf write_end1 [] = {{2, 1, SEM_UNDO},      // end updating data
                                      {0, -1, SEM_UNDO},     // decr reader
                                      {1, -1, SEM_UNDO}};    // decr writer
static struct sembuf write_end2 [] = {{3, 0, SEM_UNDO}};     // wait going to read
}  // namespace semops

sysVSem::sysVSem(key_t key, int semflg) :
    key_ (key),
    semid_(semget(key_, 4, semflg)) {
  if (semid_ < 0) {
    perror("semget");
    return;
  }
  if (-1 == semop(semid_,
                  semops::sem_init,
                  sizeof(semops::sem_init)/sizeof(*semops::sem_init))) {
    perror("sem init");
    return;
  }
}

sysVSem::~sysVSem() {
  if (is_valid()) {
    if (semctl(semid_, 0, IPC_RMID, 0) != 0) {
      perror("semctl removing semaphore");
    }
  }
}

bool sysVSem::is_valid() const {
  return 0 < semid_;
}

updateSubscriber::updateSubscriber(sysVSem *sem) :
    semid_(sem->semid_){
}

updateSubscriber::~updateSubscriber(){
}

readLock::readLock(updateSubscriber *subscriber) :
    sub_(subscriber) {
  // the first read lock need to wait for data:
  
  if (subscriber->do_wait_){
    subscriber->do_wait_ = false;
    if (-1 == semop(sub_->semid_,
                    semops::read_wait,
                    sizeof(semops::read_wait)/sizeof(*semops::read_wait))){
      valid_ = false;  // TODO log this
    }
  }
  std::this_thread::yield();
  if (-1 == semop(sub_->semid_,
                  semops::read_start,
                  sizeof(semops::read_start)/sizeof(*semops::read_start))){
    valid_ = false;  // TODO log this
  }
}

readLock::~readLock(){
  if (is_valid()) {
      semop(sub_->semid_,
            semops::read_end,
            sizeof(semops::read_end)/sizeof(*semops::read_end));
      if (!sub_->is_stoped_) {
        semop(sub_->semid_,
              semops::read_wait,
              sizeof(semops::read_wait)/sizeof(*semops::read_wait));
      }
  }
}

writeLock::writeLock(sysVSem *sem) :
    semid_(sem->semid_){
  if (-1 == semop(semid_,
                  semops::write_start,
                  sizeof(semops::write_start)/sizeof(*semops::write_start))) {
    valid_ = false;
    return;
  }
  // give a chance to reader to observe data update:
  std::this_thread::yield();
}

writeLock::~writeLock(){
  if(!is_valid())
    return;
  // give a chance to reader to observe data update:
  std::this_thread::yield();
  semop(semid_,
        semops::write_end1,
        sizeof(semops::write_end1)/sizeof(*semops::write_end1));
  semop(semid_,
         semops::write_end2,
         sizeof(semops::write_end2)/sizeof(*semops::write_end2));
}

}  // namespace shmdata
