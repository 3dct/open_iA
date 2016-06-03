/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 

/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#ifndef QXTGLOBAL_H
#define QXTGLOBAL_H

/*

#include <QtGlobal>

#define QXT_VERSION 0x000700
#define QXT_VERSION_STR "0.7.0"

//--------------------------global macros------------------------------

#ifndef QXT_NO_MACROS

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof(*x))
#endif

#endif // QXT_NO_MACROS

//--------------------------export macros------------------------------

#define QXT_DLLEXPORT DO_NOT_USE_THIS_ANYMORE

#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_CORE)
#        define QXT_CORE_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_CORE_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_CORE_EXPORT
#endif // BUILD_QXT_CORE
 
#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_GUI)
#        define QXT_GUI_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_GUI_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_GUI_EXPORT
#endif // BUILD_QXT_GUI
 
#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_NETWORK)
#        define QXT_NETWORK_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_NETWORK_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_NETWORK_EXPORT
#endif // BUILD_QXT_NETWORK
 
#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_SQL)
#        define QXT_SQL_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_SQL_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_SQL_EXPORT
#endif // BUILD_QXT_SQL
 
#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_WEB)
#        define QXT_WEB_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_WEB_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_WEB_EXPORT
#endif // BUILD_QXT_WEB
 
#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_BERKELEY)
#        define QXT_BERKELEY_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_BERKELEY_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_BERKELEY_EXPORT
#endif // BUILD_QXT_BERKELEY

#if !defined(QXT_STATIC) && !defined(QXT_DOXYGEN_RUN)
#    if defined(BUILD_QXT_ZEROCONF)
#        define QXT_ZEROCONF_EXPORT Q_DECL_EXPORT
#    else
#        define QXT_ZEROCONF_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXT_ZEROCONF_EXPORT
#endif // QXT_ZEROCONF_EXPORT

#if defined(BUILD_QXT_CORE) || defined(BUILD_QXT_GUI) || defined(BUILD_QXT_SQL) || defined(BUILD_QXT_NETWORK) || defined(BUILD_QXT_WEB) || defined(BUILD_QXT_BERKELEY) || defined(BUILD_QXT_ZEROCONF)
#   define BUILD_QXT
#endif

QXT_CORE_EXPORT const char* qxtVersion();

#ifndef QT_BEGIN_NAMESPACE
#define QT_BEGIN_NAMESPACE
#endif

#ifndef QT_END_NAMESPACE
#define QT_END_NAMESPACE
#endif

#ifndef QT_FORWARD_DECLARE_CLASS
#define QT_FORWARD_DECLARE_CLASS(Class) class Class;
#endif

#define QXT_D(PUB) PUB##Private& d = qxt_d()
#define QXT_P(PUB) PUB& p = qxt_p()

*/
#define QXT_INIT_PRIVATE(PUB) qxt_d.setPublic(this);

#define QXT_DECLARE_PRIVATE(PUB) friend class PUB##Private; QxtPrivateInterface<PUB, PUB##Private> qxt_d;
#define QXT_DECLARE_PUBLIC(PUB) friend class PUB;

template <typename PUB>
class QxtPrivate
{
public:
    virtual ~QxtPrivate()
    {}
    inline void QXT_setPublic(PUB* pub)
    {
        qxt_p_ptr = pub;
    }

protected:
    inline PUB& qxt_p()
    {
        return *qxt_p_ptr;
    }
    inline const PUB& qxt_p() const
    {
        return *qxt_p_ptr;
    }
    inline PUB* qxt_ptr()
    {
        return qxt_p_ptr;
    }
    inline const PUB* qxt_ptr() const
    {
        return qxt_p_ptr;
    }

private:
    PUB* qxt_p_ptr;
};

template <typename PUB, typename PVT>
class QxtPrivateInterface
{
    friend class QxtPrivate<PUB>;
public:
    QxtPrivateInterface()
    {
        pvt = new PVT;
    }
    ~QxtPrivateInterface()
    {
        delete pvt;
    }

    inline void setPublic(PUB* pub)
    {
        pvt->QXT_setPublic(pub);
    }
    inline PVT& operator()()
    {
        return *static_cast<PVT*>(pvt);
    }
    inline const PVT& operator()() const
    {
        return *static_cast<PVT*>(pvt);
    }
    inline PVT * operator->()
    {
	return static_cast<PVT*>(pvt);
    }
    inline const PVT * operator->() const
    {
	return static_cast<PVT*>(pvt);
    }
private:
    QxtPrivateInterface(const QxtPrivateInterface&) { }
    QxtPrivateInterface& operator=(const QxtPrivateInterface&) { }
    QxtPrivate<PUB>* pvt;
};

#endif // QXT_GLOBAL

