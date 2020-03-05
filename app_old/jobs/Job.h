// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "Task.h"

namespace ospray {
  namespace job_scheduler {

    struct Job
    {
      Job(Task task);
      ~Job();

      bool isFinished() const;
      bool isValid() const;

      Nodes get();

     private:
      Task stashedTask;
      std::future<Nodes> runningJob;
      std::atomic<bool> jobFinished;
    };

  }  // namespace job_scheduler
}  // namespace ospray

#include "detail/Job.inl"