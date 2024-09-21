/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:25:03 CET 2023                                                 */


#pragma once
using std::string;
using std::string_view;
//using std::span;
//https://en.cppreference.com/w/cpp/iterator/size
//using namespace std;
template <class C> 
constexpr auto sizear(const C& c) -> decltype(c.size())
{
//   	LOGGER("sizear: container %s\n",c.data());
    return c.size();
}
template <class C> 
constexpr auto sizear(const C* c) -> decltype(c->size())
{
//   	LOGGER("sizear: container %s\n",c.data());
    return c->size();
}
template <class T, std::size_t N>
constexpr std::size_t sizear(const T (&array)[N]) noexcept
{
//LOGGER("sizear: const array %s\n",array);
    return N-1;
}
template <class T, std::size_t N>
constexpr std::size_t sizear(T (&array)[N]) noexcept
{
//LOGGER("sizear: array %s\n",array);
	return strlen(array);
}
//template <typename T, std::enable_if_t<std::is_convertible<T, char const*>::value , bool> = true >
template <typename T, std::enable_if_t<std::is_pointer<T>::value , bool> = true >
std::size_t sizear(T s) {
//	LOGGER("sizear: ptr %s\n",s);
	return strlen(s);
	}


