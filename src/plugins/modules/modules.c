/**
 * @file
 *
 * @brief Source for modules plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include "modules.h"

#include <kdberrors.h>
#include <kdbhelper.h>
#include <kdbprivate.h> // for struct _Plugin internals

/**
 * The modules backend is automatically configured by libelektra-kdb.
 * The init functions receives a mountpoint definition with a single Key '/plugin'.
 * This binary Key contains a Plugin* of which the modules plugin takes ownership.
 * During the storage phase of kdbGet() we then call the kdbGet() function of that plugin.
 * The plugin only works like this, if it is called with a parentKey of the form 'system:/elektra/modules/<plugin>' (where '<plugin>' !=
 * 'modules'). If the plugin is called with 'system:/elektra/modules/modules' it returns its own module information. If the plugin is called
 * with 'system:/elektra/modules' it does nothing. For all other parentKeys the plugin reports an error.
 */

int ELEKTRA_PLUGIN_FUNCTION (open) (Plugin * handle, Key * errorKey ELEKTRA_UNUSED)
{
	elektraPluginSetData (handle, NULL);
	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

int ELEKTRA_PLUGIN_FUNCTION (init) (Plugin * handle, KeySet * definition, Key * parentKey ELEKTRA_UNUSED)
{
	Key * pluginKey = ksLookupByName (definition, "/plugin", 0);
	if (pluginKey != NULL)
	{
		elektraPluginSetData (handle, *(Plugin **) keyValue (pluginKey));
	}
	// init as read-only
	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

int ELEKTRA_PLUGIN_FUNCTION (get) (Plugin * handle, KeySet * returned, Key * parentKey)
{
	if (!elektraStrCmp (keyName (parentKey), "system:/elektra/modules/modules"))
	{
		KeySet * contract =
			ksNew (30, keyNew ("system:/elektra/modules/modules", KEY_VALUE, "modules plugin waits for your orders", KEY_END),
			       keyNew ("system:/elektra/modules/modules/exports", KEY_END),
			       keyNew ("system:/elektra/modules/modules/exports/open", KEY_FUNC, ELEKTRA_PLUGIN_FUNCTION (open), KEY_END),
			       keyNew ("system:/elektra/modules/modules/exports/init", KEY_FUNC, ELEKTRA_PLUGIN_FUNCTION (init), KEY_END),
			       keyNew ("system:/elektra/modules/modules/exports/get", KEY_FUNC, ELEKTRA_PLUGIN_FUNCTION (get), KEY_END),
			       keyNew ("system:/elektra/modules/modules/exports/close", KEY_FUNC, ELEKTRA_PLUGIN_FUNCTION (close), KEY_END),
#include ELEKTRA_README
			       keyNew ("system:/elektra/modules/modules/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
		ksAppend (returned, contract);
		ksDel (contract);

		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}

	Key * modulesRoot = keyNew ("system:/elektra/modules", KEY_END);
	if (keyCmp (modulesRoot, parentKey) == 0)
	{
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}

	if (keyIsDirectlyBelow (modulesRoot, parentKey) != 1)
	{
		ELEKTRA_SET_INSTALLATION_ERROR (parentKey, "The 'modules' plugin is intended for internal use by 'libelektra-kdb' only.");
		keyDel (modulesRoot);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}
	keyDel (modulesRoot);

	const char * phase = elektraPluginGetPhase (handle);
	if (strcmp (phase, KDB_GET_PHASE_RESOLVER) == 0)
	{
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	else if (strcmp (phase, KDB_GET_PHASE_STORAGE) == 0)
	{
		Plugin * plugin = elektraPluginGetData (handle);
		plugin->kdbGet (plugin, returned, parentKey);
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	else
	{
		return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
	}
}

int ELEKTRA_PLUGIN_FUNCTION (close) (Plugin * handle, Key * errorKey)
{
	Plugin * plugin = elektraPluginGetData (handle);
	if (!elektraPluginClose (plugin, errorKey))
	{
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}
	elektraPluginSetData (handle, NULL);
	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

Plugin * ELEKTRA_PLUGIN_EXPORT
{
	// clang-format off
	return elektraPluginExport ("modules",
		ELEKTRA_PLUGIN_OPEN,	&ELEKTRA_PLUGIN_FUNCTION(open),
		ELEKTRA_PLUGIN_INIT,	&ELEKTRA_PLUGIN_FUNCTION(init),
		ELEKTRA_PLUGIN_GET,	&ELEKTRA_PLUGIN_FUNCTION(get),
		ELEKTRA_PLUGIN_CLOSE,	&ELEKTRA_PLUGIN_FUNCTION(close),
		ELEKTRA_PLUGIN_END);
}
