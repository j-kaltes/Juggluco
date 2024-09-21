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
/*      Thu Apr 04 20:20:10 CEST 2024                                                 */




#pragma once

template <class C> 
constexpr auto contsize(const C& c) -> decltype(c.size())
{
//   	LOGGER("contsize: container %s\n",c.data());
    return c.size();
}
template <class C> 
constexpr auto contsize(const C* c) -> decltype(c->size())
{
//   	LOGGER("contsize: container %s\n",c.data());
    return c->size();
}
template <class T, std::size_t N>
constexpr std::size_t contsize(const T (&array)[N]) noexcept
{
//LOGGER("contsize: const array %s\n",array);
    return N;
}
template <class T, std::size_t N>
constexpr std::size_t contsize(T (&array)[N]) noexcept
{
    return N;
}


