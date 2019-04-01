/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html. COPYING can be found at the root    *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page. It can also be found at      *
 * http://hdfgroup.org/HDF5/doc/Copyright.html. If you do not have           *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
*
* Created: singleton.h
* June 5 2018
* Hariharan Devarajan <hdevarajan@hdfgroup.org>
*
* Purpose:Define singleton template for making a Singleton instances of
* certain classes in Hermes.
*
*-------------------------------------------------------------------------
*/

#ifndef HERMES_SINGLETON_H
#define HERMES_SINGLETON_H
#include <iostream>
#include <memory>
/**
 * Make a class singleton when used with the class. format for class name T
 * Singleton<T>::GetInstance()
 * @tparam T
 */
template<typename T>
class Singleton {
public:
    /**
     * Members of Singleton Class
     */
    /**
     * Uses unique pointer to build a static global instance of variable.
     * @tparam T
     * @return instance of T
     */
    template <typename... Args>
    static std::shared_ptr<T> GetInstance(Args... args)
    {
        if(instance == nullptr)
            instance = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
        return instance;
    }

    /**
     * Operators
     */
    Singleton& operator= (const Singleton) = delete; /* deleting = operatos*/
    /**
     * Constructor
     */
public:
    Singleton(const Singleton&) = delete; /* deleting copy constructor. */
protected:
    static std::shared_ptr<T> instance;
    Singleton() {} /* hidden default constructor. */
};

template<typename T>
std::shared_ptr<T> Singleton<T>::instance= nullptr;

#endif //HERMES_SINGLETON_H
