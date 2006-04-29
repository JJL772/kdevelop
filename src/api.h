#ifndef _API_H_
#define _API_H_


#include "kdevapi.h"
#include "shellexport.h"

/**
API implementation.
*/
class KDEVSHELL_EXPORT API : public KDevApi
{
public:

  virtual KDevMainWindow *mainWindow() const;
  virtual KDevDocumentController *documentController() const;
  virtual KDevPluginController *pluginController() const;
  virtual KDevCore *core() const;

  static API *getInstance();

  ~API();

protected:

  API();

private:

  static API *s_instance;

};


#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
