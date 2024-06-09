# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

include_guard()

include(cmake/utils.cmake)

FetchContent_DeclareGitHubWithMirror(cpptrace
  jeremy-rifkin/cpptrace v0.6.0
  MD5=924df247ba4a151d243130060c2c0a04
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CPPTRACE_BACKEND "CPPTRACE_GET_SYMBOLS_WITH_LIBBACKTRACE=ON")
else ()
  set(CPPTRACE_BACKEND "CPPTRACE_GET_SYMBOLS_WITH_ADDR2LINE=ON")
endif ()

FetchContent_MakeAvailableWithArgs(cpptrace
  ${CPPTRACE_BACKEND}
)
