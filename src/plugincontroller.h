#ifndef __PLUGINCONTROLLER_H__
#define __PLUGINCONTROLLER_H__

#include <qobject.h>
#include <qdict.h>
#include <qvaluelist.h>

#include <kservice.h>

class KXMLGUIClient;
class KService;
class KDevPlugin;
class KDialogBase;
class ProjectInfo;

class PluginController : public QObject
{
  Q_OBJECT

public:

  ~PluginController();

  static PluginController *getInstance();
  static KService::List pluginServices( const QString &scope = QString::null );
  static QStringList argumentsFromService( const KService::Ptr &service );

  QString currentProfile() const { return m_profile; }
  QString currentProfilePath() const { return m_profilePath; }
  
  void loadInitialPlugins();
  
  void loadLocalParts( ProjectInfo*, QStringList const & loadPlugins, QStringList const & ignorePlugins );	// @todo figure out a way to remove the ProjectInfo parameter
//  void unloadAllLocalParts();
//  void unloadLocalParts( QStringList const & );
  
  //  KDevPlugin * getPlugin( const KService::Ptr &service );
  
  void unloadPlugins( QStringList const & );
  
  void integratePart(KXMLGUIClient *part);
  void removePart(KXMLGUIClient* part);

  const QValueList<KDevPlugin*> loadedPlugins();

signals:
  void loadingPlugin(const QString &plugin);

protected:
  PluginController();

private slots:
  void slotConfigWidget( KDialogBase* );
  void loadGlobalPlugins();
  void loadCorePlugins();
//  void unloadGlobalPlugins();
  void unloadPlugins();
  
private:
	//	static void createInstance();
	static KDevPlugin *loadPlugin( const KService::Ptr &service );

	void loadDefaultParts();
  bool checkNewService( ProjectInfo *, const KService::Ptr &service );

  QDict<KDevPlugin> m_parts;
//  QDict<KDevPlugin> m_globalParts;
//  QDict<KDevPlugin> m_localParts;
  QString m_profile;
  QString m_profilePath;
  QString m_defaultProfile;
  QString m_defaultProfilePath;
  
  static PluginController *s_instance;

};

#endif
