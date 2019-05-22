// Copyright 2019 Hariharan Devarajan
/*-------------------------------------------------------------------------
 *
 * Created: debug.h
 * June 5 2018
 * Hariharan Devarajan <hdevarajan@hdfgroup.org>
 *
 * Purpose: Defines debug macros for Basket.
 *
 *-------------------------------------------------------------------------
 */

#ifndef INCLUDE_BASKET_COMMON_DEBUG_H_
#define INCLUDE_BASKET_COMMON_DEBUG_H_

#include <unistd.h>
#include <execinfo.h>
#include <iostream>
#include <csignal>


/**
 * Handles signals and prints stack trace.
 *
 * @param sig
 */
inline void handler(int sig) {
  void *array[10];
  size_t size;
  // get void*'s for all entries on the stack
  size = backtrace(array, 300);
  int rank, comm_size;
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(0);
}
/**
 * various macros to print variables and messages.
 */

#ifdef BASKET_DEBUG
#define DBGVAR(var)                                             \
  std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") "   \
  << #var << " = [" << (var) << "]" << std::endl

#define DBGVAR2(var1, var2)                                     \
  std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") "   \
  << #var1 << " = [" << (var1) << "]"                           \
  << #var2 << " = [" << (var2) << "]"  << std::endl
#define DBGVAR3(var1, var2, var3)                               \
  std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") "   \
  << #var1 << " = [" << (var1) << "]"                           \
  << #var2 << " = [" << (var2) << "]"                           \
  << #var3 << " = [" << (var3) << "]"  << std::endl

#define DBGMSG(msg)                                             \
  std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") "   \
  << msg << std::endl
#else
#define DBGVAR(var)
#define DBGVAR2(var1, var2)
#define DBGVAR3(var1, var2, var3)
#define DBGMSG(msg)
#endif

/**
 * Time all functions and instrument it
 */

#include <stdarg.h>
#include <mpi.h>
#include <stack>
#include <string>
#include <chrono>
#include <sstream>

class Timer {
 public:
  Timer():elapsed_time(0) {}
  void resumeTime() {
    t1 = std::chrono::high_resolution_clock::now();
  }
  double pauseTime() {
    auto t2 = std::chrono::high_resolution_clock::now();
    elapsed_time +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            t2 - t1).count()/1000000.0;
    return elapsed_time;
  }
  double getElapsedTime() {
    return elapsed_time;
  }
 private:
  std::chrono::high_resolution_clock::time_point t1;
  double elapsed_time;
};
/**
 * Implement Auto tracing Mechanism.
 */
using std::cout;
using std::endl;
using std::string;

using namespace std;
class AutoTrace
{
    Timer timer;
    static int rank,item;

public:
    template <typename... Args>
    AutoTrace(std::string string,Args... args):m_line(string)
    {
        char thread_name[256];
        pthread_getname_np(pthread_self(), thread_name,256);
        std::stringstream stream;

        if(rank == -1) MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#if defined(HERMES_TRACE) || defined(HERMES_TIMER)
        stream << "\033[31m";
        stream <<++item<<";"<<thread_name<<";"<< rank << ";" <<m_line << ";";
#endif
#if  defined(HERMES_TIMER)
        stream <<";;";
#endif
#ifdef HERMES_TRACE
        auto args_obj = std::make_tuple(args...);
        const ulong args_size = std::tuple_size<decltype(args_obj)>::value;
        stream << "args(";

        if(args_size == 0) stream << "Void";
        else{
            static_for<args_size>( [&](auto w){
                    stream << std::get<w.n>(args_obj) << ", ";
            });
        }
        stream << ");";
#endif
#if defined(HERMES_TRACE) || defined(HERMES_TIMER)
        stream <<"start"<< endl;
        stream << "\033[00m";
        cout << stream.str();
#endif
#ifdef HERMES_TIMER
        timer.startTime();
#endif
    }

    ~AutoTrace()
    {
        std::stringstream stream;
        char thread_name[256];
        pthread_getname_np(pthread_self(), thread_name,256);
        stream << "\033[31m";
#if defined(HERMES_TRACE) || defined(HERMES_TIMER)
        stream <<item-- <<";"<<std::string(thread_name)<<";"<< rank << ";" << m_line << ";";
#endif
#if defined(HERMES_TRACE)
        stream  <<";";
#endif
#ifdef HERMES_TIMER
        double end_time=timer.endTime();
        stream  <<end_time<<";msecs;";
#endif
#if defined(HERMES_TRACE) || defined(HERMES_TIMER)
        stream  <<"finish"<< endl;
        stream << "\033[00m";
        cout << stream.str();
#endif
    }
private:
    string m_line;
};



#endif  // INCLUDE_BASKET_COMMON_DEBUG_H_
