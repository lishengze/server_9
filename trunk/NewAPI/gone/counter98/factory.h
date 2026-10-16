/** 
  *  Copyright (c) 2018 Archforce Financial Technology.  All rights reserved.
  *  Redistribution and use in source and binary forms, with or without  modification, are not permitted.   
  *  For more information about Archforce, welcome to archforce.cn.
  **/
/**
 * @brief 工厂模式
 */

#ifndef MP_FACTORY_H_
#define MP_FACTORY_H_

#include <map>
#include <functional>
#include <mutex>
#include <unordered_map>
#include "mp_export.h"

namespace mp
{
	class FactoryClass1
	{

	};
	struct MP_EXPORT TypeIDVerKey
	{
		int64_t id ;
		int64_t ver ;
		bool operator < (const TypeIDVerKey& typeid_ver) const;

		bool operator == (const TypeIDVerKey& typeid_ver) const;
	};

} // namespace mp

// ---- std::hash 特化: 使 std::unordered_map<TypeIDVerKey, ...> 可用 ----
// 必须放在头文件中, 否则其他编译单元看不到此特化, 导致 incomplete type 错误
namespace std
{
	template<>
	struct hash<mp::TypeIDVerKey>
	{
		std::size_t operator()(const mp::TypeIDVerKey& key) const
		{
			std::size_t seed = 0;
			seed ^= std::hash<int64_t>()(key.id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<int64_t>()(key.ver) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}

namespace mp
{

	//extern MP_EXPORT std::map<std::string,void*> g_factory_instance_list_ ;	
	//extern MP_EXPORT std::mutex g_factory_mutex_ ;
	MP_EXPORT std::unordered_map<std::string, void*>& GetFactoryInstanceList();

	MP_EXPORT std::mutex& GetFactoryMutex();

/**
 *  工厂模式
 */
template <typename Type, typename type_id, typename module_name=FactoryClass1>
class Factory 
{
public:
	typedef std::function <Type*()> ConstructorType;

	static Type* Create(const type_id& id,const type_id& ver=0)
    {
        return Instance()->CreateObj(id,ver);
    }

	static void Register(const type_id& id, const type_id& ver,const ConstructorType& ctor)
	{
		Instance()->RegisterConstructor(id,ver, ctor);
	}

  private:
	Factory() {}
	static Factory<Type, type_id, module_name>* Instance()
    {
		auto factory_func = []()->Factory<Type,type_id,module_name>* {
			std::string factory_name = typeid(Factory<Type,type_id,module_name>).name() ;
			std::lock_guard<std::mutex> lock(GetFactoryMutex());
			auto iter = GetFactoryInstanceList().find(factory_name);
			decltype(iter->second) factory_ptr = nullptr;
			if( iter == GetFactoryInstanceList().end())
			{
				factory_ptr = new Factory<Type,type_id,module_name>();
				assert(factory_ptr != nullptr) ;
				GetFactoryInstanceList().emplace(std::make_pair(factory_name, factory_ptr));
			}
			else
			{
				factory_ptr = iter->second;
			}
			
			return (Factory<Type,type_id,module_name>*)factory_ptr;
		} ;

		static Factory<Type,type_id,module_name> *instance = factory_func() ;
		
        return instance ;
    }
	void RegisterConstructor(const type_id& id,const type_id& ver, const ConstructorType& ctor)
    {
        RegisterInfo info;
        info.ctor = ctor;
		TypeIDVerKey id_ver ;
		id_ver.id = id ;
		id_ver.ver = ver ;
        class_map_[id_ver] = info;
//std::cout<<"***"<<__FILE__<<"..."<<__FUNCTION__<<":id="<<id<<",ver="<<ver<<std::endl;


    }
	Type* CreateObj(const type_id& id,const type_id& ver)
    {
		TypeIDVerKey id_ver ;
		id_ver.id = id ;
		id_ver.ver = ver ;

//std::cout<<"***"<<__FILE__<<"..."<<__FUNCTION__<<":begin to find id="<<id<<",ver="<<ver<<std::endl;


        auto iter = class_map_.find(id_ver);
		if (iter != class_map_.end())
		{
//std::cout<<"***Found~ id="<<id<<",ver="<<ver<<std::endl;
			return iter->second.ctor();
		}
        else
        {
//std::cout<<"***Failed to find ~ id="<<id<<",ver="<<ver<<std::endl;
            return NULL;
        }
    }

	void Clear()
	{
		class_map_.clear();
	}

    struct RegisterInfo
    {
        ConstructorType ctor;
    };

	
	
	std::unordered_map<TypeIDVerKey, RegisterInfo> class_map_;
};


template<typename Type, typename DerivedType, typename type_id, typename module_name = FactoryClass1>
class ClassRegister
{
public:
	explicit ClassRegister(const type_id& id,const type_id ver=0)
	{
		Factory<Type, type_id, module_name>::Register(id,ver,
			[]() -> Type* { return new DerivedType(); });
	}
};


}
#endif // end of MP_FACTORY_H_
