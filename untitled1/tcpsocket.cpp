#include "tcpsocket.h"

NetworkManager* NetworkManager::instance = nullptr;

NetworkManager& NetworkManager::getInstance()
{
    if(!NetworkManager::instance)
    {
        NetworkManager::instance=new NetworkManager();
    }
    return (*NetworkManager::instance);
}

void NetworkManager::destroyInstance()
{
    if(NetworkManager::instance)
    {
        delete NetworkManager::instance;
        NetworkManager::instance=nullptr;
    }
}
