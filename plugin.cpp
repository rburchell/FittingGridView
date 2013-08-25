#include "plugin.h"
#include "fittinggridview.h"

#include <qqml.h>

void FittingGridViewPlugin::registerTypes(const char *uri)
{
    // @uri FittingGridView
    qmlRegisterType<FittingGridView>(uri, 1, 0, "FittingGridView");
}
