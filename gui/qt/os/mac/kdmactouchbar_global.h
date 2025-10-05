/****************************************************************************
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com.
** All rights reserved.
**
** This file is part of the KD MacTouchBar library.
**
** This file may be distributed and/or modified under the terms of the
** GNU Lesser General Public License version 3 as published by the
** Free Software Foundation and appearing in the file LICENSE.LGPL.txt included.
**
** You may even contact us at info@kdab.com for different licensing options.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact info@kdab.com if any conditions of this licensing are not
** clear to you.
**
**********************************************************************/
#ifndef KDMACTOUCHBAR_GLOBAL_H
#define KDMACTOUCHBAR_GLOBAL_H

#include <QtCore/QtGlobal>

#ifdef KDMACTOUCHBAR_BUILD_KDMACTOUCHBAR_LIB
# define KDMACTOUCHBAR_EXPORT Q_DECL_EXPORT
#else
# define KDMACTOUCHBAR_EXPORT Q_DECL_IMPORT
#endif

#endif /* KDMACTOUCHBAR_GLOBAL_H */
