/*******************************************************************************
 |    bberror.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bberror.h"

using namespace std;

namespace pt = boost::property_tree;

// Helper functions

int saveChild(const string& pPath)
{
    int l_SaveChild = 0;

    if (!pPath.empty() && pPath.find_first_not_of("0123456789") != string::npos)
    {
        // Not the contribid 'subtree' for bbcmd using SSH...
        for(auto& k : KEYS_TO_REMAIN)
        {
            if (k == pPath)
            {
                l_SaveChild = 1;
                break;
            }
        }
    }
    else
    {
        // Contribid 'subtree' for bbcmd using SSH...
        l_SaveChild = 1;
    }

    return l_SaveChild;
}

int BBHandler::pruneChildren(pt::ptree& pTree, const string& pPath, int pDepth)
{
    int l_PruneItem = 0;
    string l_KeyToPrune;

    if (pDepth < 2)
    {
        BOOST_FOREACH(const pt::ptree::value_type &l_Key, pTree.get_child(""))
        {
            // l_Key.first is the name of the child.
            // l_Key.second is the child subtree.
            if (!saveChild(l_Key.first))
            {
                LOG(bb,debug) << "Found child node \"" << l_Key.first << "\" to prune at path \"" << pPath << "\", depth " << pDepth;
                l_KeyToPrune = l_Key.first;
                l_PruneItem = 1;
                break;
            }
            else
            {
                LOG(bb,debug) << "Child node \"" << l_Key.first << "\" will be saved at path \"" << pPath << "\", depth " << pDepth;
            }

            if (!l_Key.second.empty())
            {
                // NOTE:  Currently, we only prune at depth level 1.  However, we still
                //        have the code below to do lower depth pruning if we ever need
                //        it in the future...
                string l_Path = (pPath.empty() ? l_Key.first : pPath + "." + l_Key.first);
                pt::ptree l_Subtree = l_Key.second;
                int l_ItemPruned = pruneChildren(l_Subtree, l_Path, pDepth+1);
                if (l_ItemPruned)
                {
                    break;
                }
            }
            else
            {
                LOG(bb,debug) << "No children exist for node \"" << l_Key.first << "\", depth " << pDepth;
            }
        }

        if (l_PruneItem)
        {
            LOG(bb,debug) << "Pruning child \"" << l_KeyToPrune << "\" at path \"" << pPath << "\", depth " << pDepth;
            errstate.get_child(pPath).erase(l_KeyToPrune);
        }
    }
    else
    {
        LOG(bb,debug) << "Pruning depth of " << pDepth << " at path \"" << pPath << "\" exceeds the maximum prune depth";
    }

    return l_PruneItem;
}

// Static methods (public)

// Instance methods (public)
int BBHandler::clear(const char* name)
{
    return clear(std::string(name));
};

int BBHandler::clear(const std::string& pConnectionName)
{
    if(!ok2Clear) return -1;
    // Store the connection name to use...
    // NOTE:  This must be done before any add() invocations...
    connectionName = pConnectionName;
    replace(connectionName.begin(), connectionName.end(), '.', '_');
    // NOTE:  We do not log any bberror without a connection name.
    if (!connectionNameIsEmpty())
    {
        if ((errstate.get("rc", "0") != "0") && config.get(process_whoami+".bringup.dump_bberror_onclear", 0))
        {
            std::string result = get("json");
            if (std::count(result.begin(), result.end(), ':') != 2)
            {
                int l_RC = rc();
                switch (l_RC)
                {
                    case 0:
                    {
                        LOG(bb, info) << "bberror: " << result;
                    }
                    break;

                    case -2:
                    case -107:
                    {
                        LOG(bb, warning) << "bberror: " << result;
                    }
                    break;

                    default:
                    {
                        LOG(bb, error) << "bberror: " << result;
                    }
                }
            }
        }
    }

    // Increment the id value for the 'next' bberror property tree...
    id.incr();

    // Clear and initialize the 'next' bberror property tree...
    // NOTE: "id" and "rc" do not have to be prepared first.
    //       Both are top level data items...
    errstate.clear();
    errstate.put("id", id.get());
    errstate.put("rc", 0);

    return 0;
};

void BBHandler::forceClear()
{
    // NOTE: If coming through the API interface, the ok2Clear indicator will be off.
    //       Otherwise, it will be on.  Preserve this attribute on exit.
    bool l_Ok2Clear = ok2Clear;
    resetToClear();
    clear();
    if (!l_Ok2Clear)
    {
        bberror.setToNotClear();
    }

    return;
}

void BBHandler::prune()
{
    string l_Path;
    pt::ptree l_Subtree;
    int l_KeepPruning = true;

    while (l_KeepPruning)
    {
        l_Path = "";
        l_Subtree = errstate.get_child(l_Path);
        LOG(bb,debug) << "prune():  At root of tree...";
        l_KeepPruning = pruneChildren(l_Subtree, l_Path, 1);
    }

   return;
}
