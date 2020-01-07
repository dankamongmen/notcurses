#ifndef __NCPP_INTERNAL_HELPERS_HH
#define __NCPP_INTERNAL_HELPERS_HH

#include <functional>
#include <map>
#include <mutex>

namespace ncpp::internal
{
	class Helpers
	{
	public:
		template<typename TKey, typename TValue>
		static TValue lookup_map_entry (std::map<TKey,TValue> *&_map, std::mutex &_mutex, TKey _key, std::function<TValue (TKey)> create_value)
		{
			std::lock_guard<std::mutex> lock (_mutex);
			if (_map == nullptr) {
				_map = new std::map<TKey,TValue> ();
			}

			TValue ret;
			auto entry = _map->find (_key);
			if (entry == _map->end ()) {
				ret = create_value (_key);
			} else {
				ret = entry->second;
			}

			return ret;
		}

		template<typename TKey, typename TValue>
		static void remove_map_entry (std::map<TKey,TValue> *&_map, std::mutex &_mutex, TKey _key)
		{
			std::lock_guard<std::mutex> lock (_mutex);
			if (_map == nullptr)
				return;

			auto entry = _map->find (_key);
			if (entry == _map->end ())
				return;

			_map->erase (entry);
		}
	};
}
#endif
