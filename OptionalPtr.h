#pragma once

#include <type_traits>

#include "CoreMinimal.h"


/**
 * 
 */
template<typename ObjectType>
class TOptionalPtr
{
#define METHOD_ASSERTS() \
	static_assert(std::is_base_of<member_type_of_t<FuncType>, std::remove_cv_t<ObjectType>>::value,\
		"Object type of the used member is not base type of the wrapped object.");\
	static_assert(!std::is_const<ObjectType>::value || is_const_method<FuncType>::value,\
		"Cannot call non-const method on const object.");

#define FIELD_ASSERTS() \
	static_assert(std::is_base_of<member_type_of_t<FieldType>, std::remove_cv_t<ObjectType>>::value,\
		"Object type of the used member is not base type of the wrapped object.");

template<typename FieldType> 
using result_of_field_t = std::remove_pointer_t<std::remove_reference_t<std::result_of_t<FieldType(ObjectType*)>>>;

template<typename FuncType, typename... Args>
using result_of_method_t = std::remove_pointer_t<std::result_of_t<FuncType(ObjectType*, Args...)>>;

//https://stackoverflow.com/questions/30407754/how-to-test-if-a-method-is-const
template<typename FuncType>
struct is_const_method;

template<typename FuncObjectType, typename ReturnType, typename... Args>
struct is_const_method<ReturnType(FuncObjectType::*)(Args...)>{
	static constexpr bool value = false;
};

template<typename FuncObjectType, typename ReturnType, typename... Args>
struct is_const_method<ReturnType(FuncObjectType::*)(Args...) const>{
	static constexpr bool value = true;
};
//END

template<typename MemberType>
struct member_type_of
{
	using type = MemberType;
};

template<typename FuncObjectType, typename ReturnType, typename... Args>
struct member_type_of<ReturnType(FuncObjectType::*)(Args...)>
{
	using type = FuncObjectType;
};
	
template<typename FuncObjectType, typename ReturnType, typename... Args>
struct member_type_of<ReturnType(FuncObjectType::*)(Args...) const>
{
	using type = FuncObjectType;
};

template<typename FieldObjectType, typename ReturnType>
struct member_type_of<ReturnType(FieldObjectType::*)>
{
	using type = FieldObjectType;
};

template<typename MemberType>
using member_type_of_t = typename member_type_of<MemberType>::type;

public:
	TOptionalPtr(ObjectType* obj) : m_obj{obj}
	{
		static_assert(!std::is_pointer<std::remove_pointer_t<decltype(obj)>>::value,
			"Argument of the Of function can be only single pointer.");
	}

	/**  
	 * @return false if wrapped object is nullptr or not valid, true otherwise  
	 */
	bool IsSet() const
	{
		return IsValidObj(m_obj);
	}

	/**
	 * @brief Applies given member function to the wrapped object and returns result wrapped in TOptionalPtr
	 * @tparam Args types of arguments provided to the member function (auto-deduced)
	 * @tparam FuncType type of member function (auto-deduced)
	 * @tparam ReturnType type of return object (auto-deduced)
	 * @param func member function to apply on the wrapped object
	 * @param args arguments provided to the member function
	 * @return result of the member function wrapped in TOptionalPtr
	 */
	template<typename... Args, typename FuncType, typename = std::enable_if_t<std::is_member_function_pointer<FuncType>::value>,
		typename ReturnType = result_of_method_t<FuncType, Args...>>
	TOptionalPtr<ReturnType> Map(FuncType&& func, Args&&... args)
	{
		METHOD_ASSERTS()
		
		return IsSet() ?
			TOptionalPtr<ReturnType>((m_obj->*func)(std::forward<Args>(args)...)) :
			TOptionalPtr<ReturnType>(nullptr);
	}

	/**
	 * @brief Applies given member field to the wrapped object and returns result wrapped in TOptionalPtr
	 * @tparam FieldType type of member field (auto-deduced)
	 * @tparam ReturnType type of return object (auto-deduced)
	 * @param field member field value of which should be retrieved from the wrapped object
	 * @return result of the member field wrapped in TOptionalPtr
	 */
	template<typename FieldType, typename = std::enable_if_t<std::is_member_object_pointer<FieldType>::value>,
		typename ReturnType = result_of_field_t<FieldType>>
	TOptionalPtr<ReturnType> Map(FieldType&& field)
	{
		FIELD_ASSERTS()
		
		return IsSet() ?
			TOptionalPtr<ReturnType>(m_obj->*field) :
			TOptionalPtr<ReturnType>(nullptr);
	}

	/**
	 * @param return_obj object to return in case the wrapped one is not valid
	 * @return wrapped object in case of being valid, return_obj otherwise
	 */
	TOptionalPtr<ObjectType> OrElse(ObjectType* return_obj)
	{
		return IsSet() ?
				TOptionalPtr<ObjectType>(m_obj) :
				TOptionalPtr<ObjectType>(return_obj);
	}

	/**
	 * @brief Applies given member function to the wrapped object and returns the result if valid, default_value otherwise
	 * @tparam Args types of arguments provided to the member function (auto-deduced)
	 * @tparam FuncType type of member function (auto-deduced)
	 * @tparam ReturnType type of return object (auto-deduced)
	 * @param default_value value to return if result from member function is not valid
	 * @param func member function to apply on the wrapped object
	 * @param args arguments provided to the member function
	 * @return result of the member function if valid, default_value otherwise
	 */
	template<typename... Args, typename FuncType, typename = std::enable_if_t<std::is_member_function_pointer<FuncType>::value>,
		typename ReturnType = result_of_method_t<FuncType, Args...>>
	ReturnType MapToValue(const ReturnType& default_value, FuncType&& func, Args&&... args)
	{
		METHOD_ASSERTS()
		
		return IsSet() ?
				(m_obj->*func)(std::forward<Args>(args)...) :
				default_value;
	}

	/**
	 * @brief Applies given member field to the wrapped object and returns the result if valid, default_value otherwise
	 * @tparam FieldType type of member field (auto-deduced)
	 * @tparam ReturnType type of return object (auto-deduced)
	 * @param field member field value of which should be retrieved from the wrapped object
	 * @return result of the member field if valid, default_value otherwise
	 */
	template<typename FieldType, typename = std::enable_if_t<std::is_member_object_pointer<FieldType>::value>,
		typename ReturnType = result_of_field_t<FieldType>>
	ReturnType MapToValue(const ReturnType& default_value, FieldType&& field)
	{
		FIELD_ASSERTS()

		return IsSet() ?
				m_obj->*field :
				default_value;
	}

	/**
	 * @brief Applies given static function with the wrapped object as first argument and returns result wrapped in TOptionalPtr
	 * @tparam Args types of arguments provided to the static function (auto-deduced)
	 * @tparam FuncType type of static function (auto-deduced)
	 * @tparam ReturnType type of return object (auto-deduced)
	 * @param func static function to apply with the wrapped object as first argument
	 * @param args other arguments provided to the static function, following the wrapped object
	 * @return result of the static function wrapped in TOptionalPtr
	 */
	template<typename... Args, typename FuncType, typename ReturnType = result_of_method_t<FuncType, Args...>>
	TOptionalPtr<ReturnType> MapStatic(FuncType&& func, Args&&... args)
	{
		return IsSet() ?
				TOptionalPtr<ReturnType>(func(m_obj, std::forward<Args>(args)...)) :
				TOptionalPtr<ReturnType>(nullptr);
	}

	/**
	 * @brief Apply given member function to the wrapped object
	 * @tparam Args types of arguments provided to the member function (auto-deduced)
	 * @tparam FuncType type of member function (auto-deduced)
	 * @param func member function to apply on the wrapped object
	 * @param args arguments provided to the member function
	 */
	template<typename... Args, typename FuncType>
	void IfPresent(FuncType&& func, Args&&... args)
	{
		METHOD_ASSERTS()
		
		if (IsSet())
			(m_obj->*func)(std::forward<Args>(args)...);
	}

	/**
	 * @return wrapped object 
	 */
	ObjectType* Get()
	{
		return m_obj;
	}

	/**
	 * @param return_value value to return if wrapped object not valid
	 * @return wrapped object if valid, return_value otherwise
	 */
	ObjectType* GetOrElse(const ObjectType* return_value)
	{
		return IsSet() ?
				m_obj :
				return_value;
	}

private:
	ObjectType* m_obj;
	
	template<typename Type = ObjectType/*has to exist to compile on PS4*/, typename = std::enable_if_t<!std::is_base_of<UObject, std::remove_cv_t<Type>>::value>>
	FORCEINLINE static bool IsValidObj(const Type* obj)
	{
		return obj != nullptr;
	}

	FORCEINLINE static bool IsValidObj(const UObject* obj)
	{
		return IsValid(obj);
	}
};
