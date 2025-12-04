#ifndef __VARIANT_H__
#define __VARIANT_H__

#include <filesystem>
#include <functional>
#include <map>
#include <cstdint>
#include <type_traits>
#include <memory>
#include <tuple>

class Global {
	private:
		Global() = delete;
		~Global() = delete;
	public:
		enum type { NIL = 0, REALTYPE, HIDDEN };
		template<class T, type t = type::REALTYPE>
		static bool set(const std::filesystem::path& name, const T& v) {
			static_assert(t != type::NIL);
			static_assert(std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T>);
			auto& variable = globalMap[name];
			if (variable.typ == type::NIL) {
				variable.setAs<T, t>(v);
			}
			else {
				if (!variable.is<T, t>()) {
					return false;
				}
				variable.as<T>() = v;
			}
			return true;
		}

		template<class T, type t = type::REALTYPE>
		static T get(const std::filesystem::path& name, const T& def) {
			auto it = globalMap.find(name);
			if (it == globalMap.end()) {
				set<T, t>(name, def);
				return def;
			}
			return it->second.as<T>();
		}

		static void reset(const std::filesystem::path& name) {
			if(name.empty()) {
				globalMap.clear();
				return;
			}
			auto it = globalMap.lower_bound(name);
			while (it != globalMap.end()) {
				auto rel = it->first.lexically_relative(name);
				if (rel.empty() || *rel.begin() != std::filesystem::path("..")) {
					it = globalMap.erase(it);
				}
				else {
					break;
				}
			}
		}

		template<class T, type t = type::REALTYPE>
		static T* getp(const std::filesystem::path& name) {
			auto it = globalMap.find(name);
			if (it == globalMap.end()) return nullptr;
			return &it->second.as<T>();
		}
	private:
		template<class T>
		static T** rttid() { static T* t{}; return &t; }
		template<class T>
		static void destroy(void* p) { if constexpr (!std::is_trivially_destructible_v<T>) { static_cast<T*>(p)->~T(); } }
		template<class T>
		static void destroyp(void* p) { delete static_cast<T*>(p); }
	private:
		struct variant_t {
			type typ = type::NIL;
			size_t tid = 0;
			uint8_t data[16]{};
			decltype(destroy<int>)* destructor{};
			~variant_t() { if (destructor)destructor(data); }
			template<class T>
			T& as() { 
				if constexpr (sizeof(T) > sizeof(data)) {
					return **reinterpret_cast<T**>(data);
				}
				else {
					return *reinterpret_cast<T*>(data);
				}
			}
			template<class T, Global::type t = type::REALTYPE>
			bool is() { return (t == type::NIL || t == typ) && (tid == reinterpret_cast<size_t>(rttid<T>)); }
			template<class T, Global::type t = type::REALTYPE>
			void setAs(const T& v) {
				static_assert(t != type::NIL);
				typ = t;
				tid = reinterpret_cast<size_t>(rttid<T>());
				if constexpr (sizeof(T) > sizeof(data)) {
					as<T*>() = new T(v);
					destructor = destroyp<T>;
				}
				else {
					new (data) T(v);
					if constexpr (!std::is_trivially_destructible_v<T>) {
						destructor = destroy<T>;
					}
				}
			}
		};

		inline static std::map<std::filesystem::path, variant_t> globalMap; // order needed
};

#endif // !__VARIANT_H__
