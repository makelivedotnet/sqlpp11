/*
 * Copyright (c) 2013-2014, Roland Bock
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLPP_INTEGRAL_H
#define SQLPP_INTEGRAL_H

#include <cstdlib>
#include <cassert>
#include <sqlpp11/basic_expression_operators.h>
#include <sqlpp11/type_traits.h>
#include <sqlpp11/exception.h>
#include <sqlpp11/value_type.h>
#include <sqlpp11/assignment.h>

namespace sqlpp
{
	namespace detail
	{

		// integral value type
		struct integral
		{
			using _traits = make_traits<integral, ::sqlpp::tag::expression>;
			using _tag = ::sqlpp::tag::integral;
			using _cpp_value_type = int64_t;

			struct _parameter_t
			{
				using _value_type = integral;

				_parameter_t():
					_value(0),
					_is_null(true)
				{}

				explicit _parameter_t(const _cpp_value_type& value):
					_value(value),
					_is_null(false)
				{}

				_parameter_t& operator=(const _cpp_value_type& value)
				{
					_value = value;
					_is_null = false;
					return *this;
				}

				void set_null()
				{
					_value = 0;
					_is_null = true;
				}

				bool is_null() const
				{ 
					return _is_null; 
				}

				const _cpp_value_type& value() const
				{
					return _value;
				}

				operator _cpp_value_type() const { return _value; }

				template<typename Target>
					void _bind(Target& target, size_t index) const
					{
						target._bind_integral_parameter(index, &_value, _is_null);
					}

			private:
				_cpp_value_type _value;
				bool _is_null;
			};

			template<typename Db, bool NullIsTrivial = false>
				struct _result_entry_t
				{
					using _value_type = integral;

					_result_entry_t():
						_is_valid(false),
						_is_null(true),
						_value(0)
					{}

					void _invalidate()
					{
						_is_valid = false;
						_is_null = true;
						_value = 0;
					}

					void _validate()
					{
						_is_valid = true;
					}

					bool is_null() const
					{ 
						if (connector_assert_result_validity_t<Db>::value)
							assert(_is_valid);
						else if (not _is_valid)
							throw exception("accessing is_null in non-existing row");
						return _is_null; 
					}

					_cpp_value_type value() const
					{
						const bool null_value = _is_null and not NullIsTrivial and not connector_null_result_is_trivial_value_t<Db>::value;
						if (connector_assert_result_validity_t<Db>::value)
						{
							assert(_is_valid);
							assert(not null_value);
						}
						else
						{
							if (not _is_valid)
								throw exception("accessing value in non-existing row");
							if (null_value)
								throw exception("accessing value of NULL field");
						}
						return _value;
					}

					operator _cpp_value_type() const { return value(); }

					template<typename Target>
						void _bind(Target& target, size_t i)
						{
							target._bind_integral_result(i, &_value, &_is_null);
						}

				private:
					bool _is_valid;
					bool _is_null;
					_cpp_value_type _value;
				};

			template<typename T>
				struct _is_valid_operand
				{
					static constexpr bool value = 
						is_expression_t<T>::value // expressions are OK
						and is_numeric_t<T>::value // the correct value type is required, of course
						;
				};

			template<typename Base>
				struct expression_operators: public basic_expression_operators<Base, is_numeric_t>
			{
				template<typename T>
					plus_t<Base, value_type_t<T>, wrap_operand_t<T>> operator +(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), {t} };
					}

				template<typename T>
					minus_t<Base, value_type_t<T>, wrap_operand_t<T>> operator -(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), {t} };
					}

				template<typename T>
					multiplies_t<Base, value_type_t<T>, wrap_operand_t<T>> operator *(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), {t} };
					}

				template<typename T>
					divides_t<Base, wrap_operand_t<T>> operator /(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), {t} };
					}

				template<typename T>
					modulus_t<Base, wrap_operand_t<T>> operator %(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), {t} };
					}

				unary_plus_t<integral, Base> operator +() const
				{
					return { *static_cast<const Base*>(this) };
				}

				unary_minus_t<integral, Base> operator -() const
				{
					return { *static_cast<const Base*>(this) };
				}
			};

			template<typename Base>
				struct column_operators
				{
					template<typename T>
						auto operator +=(T t) const -> assignment_t<Base, plus_t<Base, value_type_t<T>, wrap_operand_t<T>>>
						{
							using rhs = wrap_operand_t<T>;
							static_assert(_is_valid_operand<rhs>::value, "invalid rhs assignment operand");

							return { *static_cast<const Base*>(this), { *static_cast<const Base*>(this), rhs{t} } };
						}

					template<typename T>
						auto operator -=(T t) const -> assignment_t<Base, minus_t<Base, value_type_t<T>, wrap_operand_t<T>>>
						{
							using rhs = wrap_operand_t<T>;
							static_assert(_is_valid_operand<rhs>::value, "invalid rhs assignment operand");

							return { *static_cast<const Base*>(this), { *static_cast<const Base*>(this), rhs{t} } };
						}

					template<typename T>
						auto operator /=(T t) const -> assignment_t<Base, divides_t<Base, wrap_operand_t<T>>>
						{
							using rhs = wrap_operand_t<T>;
							static_assert(_is_valid_operand<rhs>::value, "invalid rhs assignment operand");

							return { *static_cast<const Base*>(this), { *static_cast<const Base*>(this), rhs{t} } };
						}

					template<typename T>
						auto operator *=(T t) const -> assignment_t<Base, multiplies_t<Base, value_type_t<T>, wrap_operand_t<T>>>
						{
							using rhs = wrap_operand_t<T>;
							static_assert(_is_valid_operand<rhs>::value, "invalid rhs assignment operand");

							return { *static_cast<const Base*>(this), { *static_cast<const Base*>(this), rhs{t} } };
						}
				};
		};

		template<typename Db, bool NullIsTrivial>
			inline std::ostream& operator<<(std::ostream& os, const integral::_result_entry_t<Db, NullIsTrivial>& e)
			{
				return os << e.value();
			}
	}

	using tinyint = detail::integral;
	using smallint = detail::integral;
	using integer = detail::integral;
	using bigint = detail::integral;

}
#endif
