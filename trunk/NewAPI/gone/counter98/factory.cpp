#include "factory.h"

namespace mp
{
    //MP_EXPORT std::map<std::string,void*> g_factory_instance_list_;
    //MP_EXPORT std::mutex g_factory_mutex_;
	std::unordered_map<std::string, void*>& GetFactoryInstanceList()
	{
		static std::unordered_map<std::string, void*> g_factory_instance_list;
		return g_factory_instance_list;
	}

	std::mutex& GetFactoryMutex()
	{
		static std::mutex g_factory_mutex_;
		return g_factory_mutex_;
	}

/* 	std::size_t hash_value(const TypeIDVerKey& key)
	{
		std::size_t seed = 0 ;
		boost::hash_combine(seed,key.id);
		boost::hash_combine(seed,key.ver);
		return seed;
	} */

	bool TypeIDVerKey::operator < (const TypeIDVerKey& typeid_ver) const
	{
		if (id < typeid_ver.id)
		{
			return true;
		}
		else if (id > typeid_ver.id)
		{
			return false;
		}
		else
		{
			return ver < typeid_ver.ver ;
		}
	}

	bool TypeIDVerKey::operator == (const TypeIDVerKey& typeid_ver) const
	{
		return (id == typeid_ver.id) && (ver == typeid_ver.ver);
	}

}
