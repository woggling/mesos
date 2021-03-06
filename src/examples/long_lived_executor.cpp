/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pthread.h>

#include <cstdlib>
#include <iostream>

#include <tr1/functional>

#include <mesos/executor.hpp>

using namespace mesos;
using namespace std;
using namespace std::tr1;


void run(ExecutorDriver* driver, const TaskDescription& task)
{
  sleep(100);

  TaskStatus status;
  status.mutable_task_id()->MergeFrom(task.task_id());
  status.set_state(TASK_FINISHED);

  driver->sendStatusUpdate(status);
}


void* start(void* arg)
{
  function<void(void)>* thunk = (function<void(void)>*) arg;
  (*thunk)();
  delete thunk;
  return NULL;
}


class MyExecutor : public Executor
{
public:
  virtual ~MyExecutor() {}

  virtual void init(ExecutorDriver*, const ExecutorArgs& args)
  {
    cout << "Initalized executor on " << args.hostname() << endl;
  }

  virtual void launchTask(ExecutorDriver* driver, const TaskDescription& task)
  {
    cout << "Starting task " << task.task_id().value() << endl;

    function<void(void)>* thunk =
      new function<void(void)>(bind(&run, driver, task));

    pthread_t pthread;
    if (pthread_create(&pthread, NULL, &start, thunk) != 0) {
      TaskStatus status;
      status.mutable_task_id()->MergeFrom(task.task_id());
      status.set_state(TASK_FAILED);

      driver->sendStatusUpdate(status);
    } else {
      pthread_detach(pthread);

      TaskStatus status;
      status.mutable_task_id()->MergeFrom(task.task_id());
      status.set_state(TASK_RUNNING);

      driver->sendStatusUpdate(status);
    }
  }

  virtual void killTask(ExecutorDriver* driver, const TaskID& taskId) {}

  virtual void frameworkMessage(ExecutorDriver* driver,
                                const string& data) {}

  virtual void shutdown(ExecutorDriver* driver) {}

  virtual void error(ExecutorDriver* driver, int code,
                     const std::string& message) {}
};


int main(int argc, char** argv)
{
  MyExecutor exec;
  MesosExecutorDriver driver(&exec);
  driver.run();
  return 0;
}
