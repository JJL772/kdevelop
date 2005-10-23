/***************************************************************************
 *   Copyright (C) 2002 by Bernd Gehrmann and Harald Fernengel             *
 *   bernd@kdevelop.org, harry@kdevelop.org                                *
 *   Copyright (C) 2004-2005 by Sascha Cunz                                *
 *   sascha@kdevelop.org                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "misc.h"

#include <QString>
#include <QDir>
#include <QFileInfo>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <kemailsettings.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfig.h>

void AppWizardUtil::guessAuthorAndEmail(QString *author, QString *email)
{
  KEMailSettings emailConfig;
  emailConfig.setProfile( emailConfig.defaultProfileName() );
  QString fromAddr = emailConfig.getSetting( KEMailSettings::EmailAddress );
  QString name = emailConfig.getSetting( KEMailSettings::RealName );

  if ( !fromAddr.isEmpty() && !name.isEmpty() ) {
    *author = name;
    *email = fromAddr;
    return;
  }

  struct passwd *pw = ::getpwuid(getuid());
  // pw==0 => the system must be really fucked up
  if (!pw)
      return;

  char hostname[512];

  // I guess we don't have to support users with longer host names ;-)
  (void) ::gethostname(hostname, sizeof hostname);

  if ( name.isEmpty() )
    *author = QString::fromLocal8Bit( pw->pw_gecos );
  else
    *author = name;
  if ( fromAddr.isEmpty() )
    *email = QString(pw->pw_name) + "@" + hostname;
  else
    *email = fromAddr;
}

QString AppWizardUtil::kdevRoot(const QString &templateName)
{
    QFileInfo finfo(templateName);
    QDir dir(finfo.dir());
    dir.cdUp();
    return dir.absPath();
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
