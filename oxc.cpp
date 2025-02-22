/*
    MIT License

    Copyright (c) 2025 Vladimir Abramov <abramov7613@yandex.ru>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#include "oxc.h"
#include <algorithm>                                       // for copy, tran...
#include <array>                                           // for array, arr...
#include <boost/multiprecision/cpp_int.hpp>                // for cpp_int_ba...
#include <compare>                                         // for common_com...
#include <cstdlib>                                         // for abs, size_t
#include <exception>                                       // for exception
#include <initializer_list>                                // for initialize...
#include <iterator>                                        // for back_inser...
#include <limits>                                          // for numeric_li...
#include <map>                                             // for operator==
#include <queue>                                           // for queue
#include <set>                                             // for set
#include <stdexcept>                                       // for runtime_error
#include <type_traits>                                     // for enable_if<...
#include <unordered_map>                                   // for unordered_map

// uncomment next line to disable assert()
//#define NDEBUG
#include <cassert>

/*----------------------------------------------*/
/*                  CONSTANTS                   */
/*----------------------------------------------*/

constexpr auto M_COUNT = 12;// day_markers array size
constexpr auto EMPTY_CJDN = -1;
constexpr auto MIN_CJDN_VALUE = 1721791;
const char* invalid_date = "ошибка определения даты";

/*----------------------------------------------*/
/*                   TYPE ALIAS                 */
/*----------------------------------------------*/

using ShortDate = std::pair<oxc::Month, oxc::Day> ;
using ApEvReads = oxc::OrthodoxCalendar::ApostolEvangelieReadings ;
using big_int = boost::multiprecision::cpp_int;
using INT = big_int;

/*----------------------------------------------*/
/*                  FUNCTIONS                   */
/*----------------------------------------------*/

big_int string_to_big_int(const std::string& i)
{
  big_int res;
  try { res.assign(i); }
  catch(const std::exception& e) {
    throw  std::runtime_error("ошибка преобразования строки '"+i+"' в число");
  }
  return res;
}

big_int string_to_year(const std::string& i)
{
  auto res = string_to_big_int(i);
  if( res < oxc::MIN_YEAR_VALUE )
    throw std::out_of_range("выход числа года '"+res.str()+"' за границу диапазона");
  return res;
}

namespace oxc {

bool is_leap_year(const Year& y, const CalendarFormat fmt)
{
  big_int year { string_to_big_int(y) };
  switch(fmt){
    case Grigorian: return (year%400 == 0) || (year%100 != 0 && year%4 == 0) ;
    case Julian: return (year%4 == 0) ;
    case Milankovic: {
      if(year%4 == 0) {
        if(year%100 == 0) {
          int x = boost::multiprecision::integer_modulus(year/100, 9);
          if(x == 2 || x == 6) return true;
          else return false;
        }
        return true;
      }
      return false;
    }
    default: return false;
  }
}

Day month_length(const Month month, const bool leap)
{
  switch(month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
        break;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
        break;
    case 2:
        return leap ? 29 : 28;
        break;
    default:
        return 0;
  }
}

std::string property_title(oxc_const property)
{
  static const std::map<uint16_t, std::string_view> table = {
  //таблица - группа констант 1 - переходящие дни года
    {pasha,              "Светлое Христово Воскресение. ПАСХА."},
    {svetlaya1,          "Понедельник Светлой седмицы."},
    {svetlaya2,          "Вторник Светлой седмицы."},
    {svetlaya3,          "Среда Светлой седмицы."},
    {svetlaya4,          "Четверг Светлой седмицы."},
    {svetlaya5,          "Пятница Светлой седмицы."},
    {svetlaya6,          "Суббота Светлой седмицы."},
    {ned2_popashe,       "Неделя 2-я по Пасхе, апостола Фомы́. Антипасха."},
    {s2popashe_1,        "Понедельник 2-й седмицы по Пасхе."},
    {s2popashe_2,        "Вторник 2-й седмицы по Пасхе. Ра́доница. Поминовение усопших."},
    {s2popashe_3,        "Среда 2-й седмицы по Пасхе."},
    {s2popashe_4,        "Четверг 2-й седмицы по Пасхе."},
    {s2popashe_5,        "Пятница 2-й седмицы по Пасхе."},
    {s2popashe_6,        "Суббота 2-й седмицы по Пасхе."},
    {ned3_popashe,       "Неделя 3-я по Пасхе, святых жен-мироносиц: Марии Магдалины, Марии Клеоповой, Саломии, Иоанны, Марфы и Марии, Сусанны и иных."},
    {s3popashe_1,        "Понедельник 3-й седмицы по Пасхе."},
    {s3popashe_2,        "Вторник 3-й седмицы по Пасхе."},
    {s3popashe_3,        "Среда 3-й седмицы по Пасхе."},
    {s3popashe_4,        "Четверг 3-й седмицы по Пасхе."},
    {s3popashe_5,        "Пятница 3-й седмицы по Пасхе."},
    {s3popashe_6,        "Суббота 3-й седмицы по Пасхе."},
    {ned4_popashe,       "Неделя 4-я по Пасхе, о расслабленном."},
    {s4popashe_1,        "Понедельник 4-й седмицы по Пасхе."},
    {s4popashe_2,        "Вторник 4-й седмицы по Пасхе."},
    {s4popashe_3,        "Среда 4-й седмицы по Пасхе. Преполове́ние Пятидесятницы."},
    {s4popashe_4,        "Четверг 4-й седмицы по Пасхе."},
    {s4popashe_5,        "Пятница 4-й седмицы по Пасхе."},
    {s4popashe_6,        "Суббота 4-й седмицы по Пасхе."},
    {ned5_popashe,       "Неделя 5-я по Пасхе, о самаряны́не."},
    {s5popashe_1,        "Понедельник 5-й седмицы по Пасхе."},
    {s5popashe_2,        "Вторник 5-й седмицы по Пасхе."},
    {s5popashe_3,        "Среда 5-й седмицы по Пасхе. Отдание праздника Преполовения Пятидесятницы."},
    {s5popashe_4,        "Четверг 5-й седмицы по Пасхе."},
    {s5popashe_5,        "Пятница 5-й седмицы по Пасхе."},
    {s5popashe_6,        "Суббота 5-й седмицы по Пасхе."},
    {ned6_popashe,       "Неделя 6-я по Пасхе, о слепом."},
    {s6popashe_1,        "Понедельник 6-й седмицы по Пасхе."},
    {s6popashe_2,        "Вторник 6-й седмицы по Пасхе."},
    {s6popashe_3,        "Среда 6-й седмицы по Пасхе. Отдание праздника Пасхи. Предпразднство Вознесения."},
    {s6popashe_4,        "Четверг 6-й седмицы по Пасхе. Вознесе́ние Госпо́дне."},
    {s6popashe_5,        "Пятница 6-й седмицы по Пасхе. Попразднство Вознесения."},
    {s6popashe_6,        "Суббота 6-й седмицы по Пасхе. Попразднство Вознесения."},
    {ned7_popashe,       "Неделя 7-я по Пасхе. Попразднство Вознесения. Святых отцов Первого Вселенского Собора."},
    {s7popashe_1,        "Понедельник 7-й седмицы по Пасхе. Попразднство Вознесения."},
    {s7popashe_2,        "Вторник 7-й седмицы по Пасхе. Попразднство Вознесения."},
    {s7popashe_3,        "Среда 7-й седмицы по Пасхе. Попразднство Вознесения."},
    {s7popashe_4,        "Четверг 7-й седмицы по Пасхе. Попразднство Вознесения."},
    {s7popashe_5,        "Пятница 7-й седмицы по Пасхе. Отдание праздника Вознесения Господня."},
    {s7popashe_6,        "Суббота 7-й седмицы по Пасхе. Троицкая родительская суббота."},
    {ned8_popashe,       "Неделя 8-я по Пасхе. День Святой Тро́ицы. Пятидеся́тница."},
    {s1po50_1,           "Понедельник Пятидесятницы. День Святаго Духа."},
    {s1po50_2,           "Вторник Пятидесятницы."},
    {s1po50_3,           "Среда Пятидесятницы."},
    {s1po50_4,           "Четверг Пятидесятницы."},
    {s1po50_5,           "Пятница Пятидесятницы."},
    {s1po50_6,           "Суббота Пятидесятницы. Отдание праздника Пятидесятницы."},
    {ned1_po50,          "Неделя 1-я по Пятидесятнице, Всех святых."},
    {ned2_po50,          "Неделя 2-я по Пятидесятнице, Всех святых, в земле Русской просиявших."},
    {ned3_po50,          "Неделя 3-я по Пятидесятнице."},
    {ned4_po50,          "Неделя 4-я по Пятидесятнице."},
    {sub_pered14sent,    "Суббота пред Воздвижением."},
    {ned_pered14sent,    "Неделя пред Воздвижением."},
    {sub_po14sent,       "Суббота по Воздвижении."},
    {ned_po14sent,       "Неделя по Воздвижении."},
    {sobor_otcev7sobora, "Память святых отцов VII Вселенского Собора."},
    {sub_dmitry,         "Димитриевская родительская суббота."},
    {ned_praotec,        "Неделя святых пра́отец."},
    {sub_peredrojd,      "Суббота пред Рождеством Христовым."},
    {ned_peredrojd,      "Неделя пред Рождеством Христовым, святых отец."},
    {sub_porojdestve,    "Суббота по Рождестве Христовом."},
    {ned_porojdestve,    "Неделя по Рождестве Христовом."},
    {ned_mitar_ifaris,   "Неделя о мытаре́ и фарисе́е."},
    {ned_obludnom,       "Неделя о блудном сыне."},
    {sub_myasopust,      "Суббота мясопу́стная. Вселенская родительская суббота."},
    {ned_myasopust,      "Неделя мясопу́стная, о Страшном Суде."},
    {sirnaya1,           "Понедельник сырный."},
    {sirnaya2,           "Вторник сырный."},
    {sirnaya3,           "Среда сырная."},
    {sirnaya4,           "Четверг сырный."},
    {sirnaya5,           "Пятница сырная."},
    {sirnaya6,           "Суббота сырная."},
    {ned_siropust,       "Неделя сыропустная. Воспоминание Адамова изгнания. Прощеное воскресенье."},
    {vel_post_d1n1,      "Понедельник 1-й седмицы. Начало Великого поста."},
    {vel_post_d2n1,      "Вторник 1-й седмицы великого поста."},
    {vel_post_d3n1,      "Среда 1-й седмицы великого поста."},
    {vel_post_d4n1,      "Четверг 1-й седмицы великого поста."},
    {vel_post_d5n1,      "Пятница 1-й седмицы великого поста."},
    {vel_post_d6n1,      "Суббота 1-й седмицы великого поста."},
    {vel_post_d0n2,      "Неделя 1-я Великого поста. Торжество Православия."},
    {vel_post_d1n2,      "Понедельник 2-й седмицы великого поста."},
    {vel_post_d2n2,      "Вторник 2-й седмицы великого поста."},
    {vel_post_d3n2,      "Среда 2-й седмицы великого поста."},
    {vel_post_d4n2,      "Четверг 2-й седмицы великого поста."},
    {vel_post_d5n2,      "Пятница 2-й седмицы великого поста."},
    {vel_post_d6n2,      "Суббота 2-й седмицы великого поста."},
    {vel_post_d0n3,      "Неделя 2-я Великого поста."},
    {vel_post_d1n3,      "Понедельник 3-й седмицы великого поста."},
    {vel_post_d2n3,      "Вторник 3-й седмицы великого поста."},
    {vel_post_d3n3,      "Среда 3-й седмицы великого поста."},
    {vel_post_d4n3,      "Четверг 3-й седмицы великого поста."},
    {vel_post_d5n3,      "Пятница 3-й седмицы великого поста."},
    {vel_post_d6n3,      "Суббота 3-й седмицы великого поста."},
    {vel_post_d0n4,      "Неделя 3-я Великого поста, Крестопоклонная."},
    {vel_post_d1n4,      "Понедельник 4-й седмицы вел. поста, Крестопоклонной."},
    {vel_post_d2n4,      "Вторник 4-й седмицы вел. поста, Крестопоклонной."},
    {vel_post_d3n4,      "Среда 4-й седмицы вел. поста, Крестопоклонной."},
    {vel_post_d4n4,      "Четверг 4-й седмицы вел. поста, Крестопоклонной."},
    {vel_post_d5n4,      "Пятница 4-й седмицы вел. поста, Крестопоклонной."},
    {vel_post_d6n4,      "Суббота 4-й седмицы вел. поста, Крестопоклонной."},
    {vel_post_d0n5,      "Неделя 4-я Великого поста."},
    {vel_post_d1n5,      "Понедельник 5-й седмицы великого поста."},
    {vel_post_d2n5,      "Вторник 5-й седмицы великого поста."},
    {vel_post_d3n5,      "Среда 5-й седмицы великого поста."},
    {vel_post_d4n5,      "Четверг 5-й седмицы великого поста."},
    {vel_post_d5n5,      "Пятница 5-й седмицы великого поста."},
    {vel_post_d6n5,      "Суббота 5-й седмицы великого поста. Суббота Ака́фиста. Похвала́ Пресвятой Богородицы."},
    {vel_post_d0n6,      "Неделя 5-я Великого поста."},
    {vel_post_d1n6,      "Понедельник 6-й седмицы великого поста, ва́ий."},
    {vel_post_d2n6,      "Вторник 6-й седмицы великого поста, ва́ий."},
    {vel_post_d3n6,      "Среда 6-й седмицы великого поста, ва́ий."},
    {vel_post_d4n6,      "Четверг 6-й седмицы великого поста, ва́ий."},
    {vel_post_d5n6,      "Пятница 6-й седмицы великого поста, ва́ий."},
    {vel_post_d6n6,      "Суббота 6-й седмицы великого поста, ва́ий. Лазарева суббота."},
    {vel_post_d0n7,      "Неделя ва́ий (цветоно́сная, Вербное воскресенье). Вход Господень в Иерусалим."},
    {vel_post_d1n7,      "Страстна́я седмица. Великий Понедельник."},
    {vel_post_d2n7,      "Страстна́я седмица. Великий Вторник."},
    {vel_post_d3n7,      "Страстна́я седмица. Великая Среда."},
    {vel_post_d4n7,      "Страстна́я седмица. Великий Четверг. Воспоминание Тайной Ве́чери."},
    {vel_post_d5n7,      "Страстна́я седмица. Великая Пятница."},
    {vel_post_d6n7,      "Страстна́я седмица. Великая Суббота."},
  //таблица - группа констант 2 - непереходящие дни года
    {m1d1,  "Обре́зание Господне. Свт. Василия Великого, архиеп. Кесари́и Каппадоки́йской."},
    {m1d2,  "Предпразднство Богоявления."},
    {m1d3,  "Предпразднство Богоявления."},
    {m1d4,  "Предпразднство Богоявления."},
    {m1d5,  "Предпразднство Богоявления. На́вечерие Богоявления (Крещенский сочельник). День постный."},
    {m1d6,  "Святое Богоявле́ние. Крещение Господа Бога и Спаса нашего Иисуса Христа."},
    {m1d7,  "Попразднство Богоявления."},
    {m1d8,  "Попразднство Богоявления."},
    {m1d9,  "Попразднство Богоявления."},
    {m1d10, "Попразднство Богоявления."},
    {m1d11, "Попразднство Богоявления."},
    {m1d12, "Попразднство Богоявления."},
    {m1d13, "Попразднство Богоявления."},
    {m1d14, "Отдание праздника Богоявления."},
    {m3d25, "Благове́щение Пресвято́й Богоро́дицы."},
    {m6d24, "Рождество́ честно́го сла́вного Проро́ка, Предте́чи и Крести́теля Госпо́дня Иоа́нна."},
    {m6d25, "Отдание праздника рождества Предте́чи и Крести́теля Госпо́дня Иоа́нна."},
    {m6d29, "Славных и всехва́льных первоверхо́вных апостолов Петра и Павла."},
    {m8d5,  "Предпразднство Преображения Господня."},
    {m8d6,  "Преображение Господа Бога и Спаса нашего Иисуса Христа."},
    {m8d7,  "Попразднство Преображения Господня."},
    {m8d8,  "Попразднство Преображения Господня."},
    {m8d9,  "Попразднство Преображения Господня."},
    {m8d10, "Попразднство Преображения Господня."},
    {m8d11, "Попразднство Преображения Господня."},
    {m8d12, "Попразднство Преображения Господня."},
    {m8d13, "Отдание праздника Преображения Господня."},
    {m8d14, "Предпразднство Успения Пресвятой Богородицы."},
    {m8d15, "Успе́ние Пресвятой Владычицы нашей Богородицы и Приснодевы Марии."},
    {m8d16, "Попразднство Успения Пресвятой Богородицы."},
    {m8d17, "Попразднство Успения Пресвятой Богородицы."},
    {m8d18, "Попразднство Успения Пресвятой Богородицы."},
    {m8d19, "Попразднство Успения Пресвятой Богородицы."},
    {m8d20, "Попразднство Успения Пресвятой Богородицы."},
    {m8d21, "Попразднство Успения Пресвятой Богородицы."},
    {m8d22, "Попразднство Успения Пресвятой Богородицы."},
    {m8d23, "Отдание праздника Успения Пресвятой Богородицы."},
    {m9d7,  "Предпразднство Рождества Пресвятой Богородицы."},
    {m9d8,  "Рождество Пресвятой Владычицы нашей Богородицы и Приснодевы Марии."},
    {m9d9,  "Попразднство Рождества Пресвятой Богородицы."},
    {m9d10, "Попразднство Рождества Пресвятой Богородицы."},
    {m9d11, "Попразднство Рождества Пресвятой Богородицы."},
    {m9d12, "Отдание праздника Рождества Пресвятой Богородицы."},
    {m9d13, "Предпразднство Воздви́жения Честно́го и Животворя́щего Креста Господня."},
    {m9d14, "Всеми́рное Воздви́жение Честно́го и Животворя́щего Креста́ Госпо́дня. День постный."},
    {m9d15, "Попразднство Воздвижения Креста."},
    {m9d16, "Попразднство Воздвижения Креста."},
    {m9d17, "Попразднство Воздвижения Креста."},
    {m9d18, "Попразднство Воздвижения Креста."},
    {m9d19, "Попразднство Воздвижения Креста."},
    {m9d20, "Попразднство Воздвижения Креста."},
    {m9d21, "Отдание праздника Воздвижения Животворящего Креста Господня."},
    {m8d29, "Усекновение главы́ Пророка, Предтечи и Крестителя Господня Иоанна. День постный."},
    {m10d1, "Покро́в Пресвятой Владычицы нашей Богородицы и Приснодевы Марии."},
    {m11d20,"Предпразднство Введения (Входа) во храм Пресвятой Богородицы."},
    {m11d21,"Введе́ние (Вход) во храм Пресвятой Владычицы нашей Богородицы и Приснодевы Марии."},
    {m11d22,"Попразднство Введения."},
    {m11d23,"Попразднство Введения."},
    {m11d24,"Попразднство Введения."},
    {m11d25,"Отдание праздника Введения (Входа) во храм Пресвятой Богородицы."},
    {m12d20,"Предпразднство Рождества Христова."},
    {m12d21,"Предпразднство Рождества Христова."},
    {m12d22,"Предпразднство Рождества Христова."},
    {m12d23,"Предпразднство Рождества Христова."},
    {m12d24,"Предпразднство Рождества Христова. На́вечерие Рождества Христова (Рождественский сочельник)."},
    {m12d25,"Рождество Господа Бога и Спаса нашего Иисуса Христа."},
    {m12d26,"Попразднство Рождества Христова."},
    {m12d27,"Попразднство Рождества Христова."},
    {m12d28,"Попразднство Рождества Христова."},
    {m12d29,"Попразднство Рождества Христова."},
    {m12d30,"Попразднство Рождества Христова."},
    {m12d31,"Отдание праздника Рождества Христова."},
  //таблица - группа констант 3 - другие дни года
    {sub_peredbogoyav,        "Суббота перед Богоявлением."},
    {ned_peredbogoyav,        "Неделя перед Богоявлением."},
    {sub_pobogoyav,           "Суббота по Богоявлении."},
    {ned_pobogoyav,           "Неделя по Богоявлении."},
    {sobor_novom_rus,         "Собор новомучеников и исповедников Церкви Русской."},
    {sobor_3sv,               "Собор вселенских учителей и святителей Василия Великого, Григория Богослова и Иоанна Златоустого."},
    {sretenie_predpr,         "Предпразднство Сре́тения Господня."},
    {sretenie,                "Сре́тение Господа Бога и Спаса нашего Иисуса Христа."},
    {sretenie_poprazd1,       "День 1-й Попразднства Сретения Господня."},
    {sretenie_poprazd2,       "День 2-й Попразднства Сретения Господня."},
    {sretenie_poprazd3,       "День 3-й Попразднства Сретения Господня."},
    {sretenie_poprazd4,       "День 4-й Попразднства Сретения Господня."},
    {sretenie_poprazd5,       "День 5-й Попразднства Сретения Господня."},
    {sretenie_poprazd6,       "День 6-й Попразднства Сретения Господня."},
    {sretenie_otdanie,        "Отдание праздника Сретения Господня."},
    {obret_gl_ioanna12,       "Первое и второе Обре́тение главы Иоанна Предтечи."},
    {muchenik_40,             "Святых сорока́ мучеников, в Севастийском е́зере мучившихся."},
    {blag_predprazd,          "Предпразднство Благовещения Пресвятой Богородицы."},
    {blag_otdanie,            "Отдание праздника Благовещения Пресвятой Богородицы."},
    {georgia_pob,             "Вмч. Гео́ргия Победоно́сца. Мц. царицы Александры."},
    {obret_gl_ioanna3,        "Третье обре́тение главы Предтечи и Крестителя Господня Иоанна."},
    {sobor_otcev_1_6sob,      "Память святых отцов шести Вселенских Соборов."},
    {feodor_tir              ,"Вмч. Феодора Тирона (ок. 306) (переходящее празднование)."},
    {grigor_palam            ,"Свт. Григория Паламы, архиеп. Фессалонитского (переходящее празднование)."},
    {ioann_lestv             ,"Прп. Иоанна Лествичника (переходящее празднование)."},
    {mari_egipt              ,"Прп. Марии Египетской (переходящее празднование)."},
    {sub_porojdestve_r       ,"Чтения субботы по Рождестве Христовом."},
    {ned_porojdestve_r       ,"Чтения недели по Рождестве Христовом."},
    {sub_peredbogoyav_r      ,"Чтения субботы пред Богоявлением."},
    {ned_peredbogoyav_r      ,"Чтения недели пред Богоявлением."},
    {ned_prav_bogootec       ,"Правв. Иосифа Обручника, Давида царя и Иакова, брата Господня."},
    {sobor_vsehsv_rus        ,"Всех святых, в земле Русской просиявших."},
  //таблица - группа констант 4 - типы праздников
    { dvana10_per_prazd,       "Двунадесятые переходящие праздники"},
    { dvana10_nep_prazd,       "Двунадесятые непереходящие праздники"},
    { vel_prazd,               "Великие праздники"},
  //таблица - группа констант 5 - посты и сплошные седмицы
    { post_vel,               "Великий пост"},
    { post_petr,              "Петров пост"},
    { post_usp,               "Успенский пост"},
    { post_rojd,              "Рождественский пост"},
    { full7_svyatki,          "Сплошная седмица. Святки"},
    { full7_mitar,            "Сплошная седмица. Мытаря и фарисея"},
    { full7_sirn,             "Сплошная седмица. Сырная (Масленица)"},
    { full7_pasha,            "Сплошная седмица. Светлая"},
    { full7_troica,           "Сплошная седмица. Троицкая"},
  //таблица - группа констант 6 - переходящие дни празднования икон Богородицы
    { mari_icon_01,               "иконы Божией Матери «Акафистная Дионисиатская (Мироточивая)»"},
    { mari_icon_02,               "иконы Божией Матери «Аз есмь с вами, и никтоже на вы (Леуши́нская)»"},
    { mari_icon_03,               "иконы Божией Матери «Девпетуровская-Тамбовская»"},
    { mari_icon_04,               "иконы Божией Матери «Дубенская (Красногорская)»"},
    { mari_icon_05,               "иконы Божией Матери «Дектоурская (Доктоурская)»"},
    { mari_icon_06,               "иконы Божией Матери «Живоносный Источник»"},
    { mari_icon_07,               "иконы Божией Матери «Межеричская (Жизнеподательница)»"},
    { mari_icon_08,               "иконы Божией Матери «Зна́мение Курская-Коренная»"},
    { mari_icon_09,               "иконы Божией Матери «Иверская»"},
    { mari_icon_10,               "иконы Божией Матери «Избавление От Бед Страждущих»"},
    { mari_icon_11,               "иконы Божией Матери «Кипрская (Стромынская)»"},
    { mari_icon_12,               "иконы Божией Матери «Кипрская»"},
    { mari_icon_13,               "иконы Божией Матери «Казанская Коробейниковская»"},
    { mari_icon_14,               "иконы Божией Матери «Моздокская (Иверская)»"},
    { mari_icon_15,               "иконы Божией Матери «Марьиногорская»"},
    { mari_icon_16,               "иконы Божией Матери «Нерушимая Стена»"},
    { mari_icon_17,               "иконы Божией Матери «Одигитрия Шуйская»"},
    { mari_icon_18,               "иконы Божией Матери «Прибавление Ума»"},
    { mari_icon_19,               "иконы Божией Матери «Споручница грешных Корецкая»"},
    { mari_icon_20,               "иконы Божией Матери «Тупичевская»"},
    { mari_icon_21,               "иконы Божией Матери «Табынская»"},
    { mari_icon_22,               "иконы Божией Матери «Умягчение Злых Сердец»"},
    { mari_icon_23,               "иконы Божией Матери «Умиление Псковско-Печерская»"},
    { mari_icon_24,               "иконы Божией Матери «Касперовская»"},
    { mari_icon_25,               "иконы Божией Матери «Челнская»"},
  //таблица - группа констант 7 - переходящие дни празднования святых
    { sobor_valaam,            "Собо́р преподо́бных отце́в, на Валаа́ме просия́вших."},
    { varlaam_hut,             "Прп. Варлаа́ма Ху́тынского (переходящее празднование)."},
    { petr_fevron_murom,       "Перенесение мощей блгвв. кн. Петра, в иночестве Давида, и кн. Февронии, в иночестве Евфросинии, Муромских чудотворцев."},
    { sobor_bessrebren,        "Собор всех Бессребреников."},
    { sobor_tversk,            "Собор Тверских святых."},
    { sobor_kuzbas,            "Собор Кузбасских святых."},
    { pahomii_kensk,           "Прп. Пахомия Кенского (XVI) (переходящее празднование)."},
    { shio_mg,                 "Прп.Шио Мгвимского (VI) (Груз.) (переходящее празднование)."},
    { prep_dav_gar,            "Преподобномучеников отцов Давидо-Гареджийских (1616) (Груз.)(переходящее празднование)."},
    { hristodul,               "Мчч. Христодула и Анастасии Патрских, убиенных в Ахаии (1821) (переходящее празднование)."},
    { iosif_arimaf,            "праведных Иосифа Аримафейского и Никодима (переходящее празднование)."},
    { tamar_gruz,              "Блгв. Тамары, царицы Грузинской (переходящее празднование)."},
    { pm_avraam_bolg,          "Перенесение мощей мч. Авраамия Болгарского (1230)(переходящее празднование)."},
    { tavif,                   "Прав. Тавифы (I)(переходящее празднование)."},
    { much_fereidan,           "Мучеников, в долине Ферейдан (Иран) от персов пострадавших (XVII) (Груз.) (переходящее празднование)."},
    { dodo_gar,                "Прп. Додо Гареджийского (Груз.)(623) (переходящее празднование)."},
    { david_gar,               "Прп. Давида Гареджийского (Груз.)(VI) (переходящее празднование)."},
    { prep_sokolovsk,          "Прпп. Тихона, Василия и Никона Соколовских(XVI) (переходящее празднование)."},
    { arsen_tversk,            "Свт.Арсения, еп. Тверского (переходящее празднование)."},
    { much_lipsiisk,           "Прмчч. Неофита, Ионы, Неофита, Ионы и Парфения Липсийских (переходящее празднование)."},
    { sobor_altai,             "Собор Алтайских святых."},
    { sobor_afonpr,            "Собор всех преподобных и Богоносных отцов, во Святой Горе Афонской просиявших"},
    { sobor_belorus,           "Собор Белорусских святых"},
    { sobor_vologod,           "Собор Вологодских святых"},
    { sobor_novgorod,          "Собор Новгородских святых"},
    { sobor_pskov,             "Собор Псковских святых"},
    { sobor_piter,             "Собор святых Санкт-Петербургской митрополии"},
    { sobor_udmurt,            "Собор святых Удмуртской земли"},
    { sobor_volgograd,         "Собор всех святых, в земле Волгоградской просиявших"},
    { sobor_ispan,             "Собор святых, в земле Испанской и Португальской просиявших"},
    { sobor_kuban,             "Собор святых Кубанской митрополии"},
    { sobor_chelyab,           "Собор святых Челябинской митрополии"},
    { sobor_mosk,              "Собор Московских святых"},
    { sobor_nnovgor,           "Собор святых Нижегородской митрополии"},
    { sobor_saratov,           "Собор Саратовских святых"},
    { sobor_butov,             "Собор новомучеников, в Бутове пострадавших"},
    { sobor_kazahst,           "Собор новомучеников и исповедников Казахстанских"},
    { sobor_karel,             "Собор новомучеников и исповедников земли Карельской"},
    { sobor_perm,              "Собор святых Пермской митрополии"},
    { sobor_ppech_prep,        "Собор преподобных отцов Псково-Печерских"},
    { sobor_sinai_prep,        "Собор преподобных отцов, на Богошественной Горе Синай подвизавшихся"},
    { sobor_much_holm,         "Собор мучеников Холмских и Подляшских"},
    { sobor_vseh_prep,         "Собор всех преподобных отцов, в подвиге просиявших"},
    { sobor_kpech_prep,        "Собор всех преподобных отцов Киево-Печерских"},
    { sobor_smolensk,          "Собор Смоленских святых"},
    { sobor_alansk,            "Собор Аланских святых"},
    { sobor_german,            "Собор святых, в земле Германской просиявших"}
  };
  if(auto x = table.find(property); x!=table.end()) return std::string(x->second);
  return {};
}

/*----------------------------------------------*/
/*              class Date::impl                */
/*----------------------------------------------*/

class Date::impl {
  INT cjdn_;                         //Chronological Julian Day Number
  std::tuple<Year,Month,Day> gdate_; //Grigorian date
  std::tuple<Year,Month,Day> jdate_; //Julian date
  std::tuple<Year,Month,Day> mdate_; //Milankovic date

  int fdiv_(int a, int b) const;
  INT fdiv_(const INT& a, const INT& b) const;
  template<typename Integer>
    Integer mod_(const Integer& a, const Integer& b) const;
  std::pair<int,int> pdiv_(int a, int b) const;
  std::pair<INT,INT> pdiv_(const INT& a, const INT& b) const;
  INT grigorian2cjdn(const Year& y, const Month m, const Day d) const;
  INT julian2cjdn(const Year& y, const Month m, const Day d) const;
  INT milankovic2cjdn(const Year& y, const Month m, const Day d) const;
  std::tuple<Year,Month,Day> cjdn2grigorian(const INT& cjdn) const;
  std::tuple<Year,Month,Day> cjdn2julian(const INT& cjdn) const;
  std::tuple<Year,Month,Day> cjdn2milankovic(const INT& cjdn) const;

public:
  impl();
  impl(const Year& y, const Month m, const Day d, const CalendarFormat f);
  impl(const INT& cjdn);
  bool reset();
  bool reset(const Year& y, const Month m, const Day d, const CalendarFormat f);
  bool reset(const INT& new_cjdn);
  bool operator==(const Date::impl& rhs) const;
  bool operator!=(const Date::impl& rhs) const;
  bool operator<(const Date::impl& rhs) const ;
  bool operator>(const Date::impl& rhs) const ;
  bool operator>=(const Date::impl& rhs) const;
  bool operator<=(const Date::impl& rhs) const;
  bool is_valid() const;
  Year year(const CalendarFormat fmt) const;
  Month month(const CalendarFormat fmt) const;
  Day day(const CalendarFormat fmt) const;
  Weekday weekday() const;
  std::tuple<Year,Month,Day> ymd(const CalendarFormat fmt) const;
  INT cjdn() const;
  INT cjdn_from_incremented_by(unsigned long long c) const;
  INT cjdn_from_decremented_by(unsigned long long c) const;
  std::string& format(std::string& fmt) const;
};

bool Date::impl::reset()
{
  gdate_ = std::make_tuple<Year,Month,Day>({},{},{});
  jdate_ = std::make_tuple<Year,Month,Day>({},{},{});
  mdate_ = std::make_tuple<Year,Month,Day>({},{},{});
  cjdn_ = EMPTY_CJDN;
  return true;
}

bool Date::impl::reset(const Year& y, const Month m, const Day d, const CalendarFormat f)
{
  if( m<1 || m>12 ) return false;
  INT x;
  try { x.assign(y); } catch(const std::exception& e) { return false; }
  if( x < MIN_YEAR_VALUE ) return false;
  if( d<1 || d > month_length(m, is_leap_year(y, f)) ) return false;
  std::tuple<Year,Month,Day> jx, gx, mx ;
  switch(f) {
    case Grigorian: {
      x = grigorian2cjdn(y, m, d);
      gx = std::make_tuple(y, m, d);
      jx = cjdn2julian(x);
      mx = cjdn2milankovic(x);
    } break;
    case Julian: {
      x = julian2cjdn(y, m, d);
      jx = std::make_tuple(y, m, d);
      gx = cjdn2grigorian(x);
      mx = cjdn2milankovic(x);
    } break;
    case Milankovic: {
      x = milankovic2cjdn(y, m, d);
      mx = std::make_tuple(y, m, d);
      gx = cjdn2grigorian(x);
      jx = cjdn2julian(x);
    } break;
    default: { return false; }
  }
  INT jy ( std::get<0>(jx) );
  INT gy ( std::get<0>(gx) );
  INT my ( std::get<0>(mx) );
  if( jy < MIN_YEAR_VALUE || gy < MIN_YEAR_VALUE || my < MIN_YEAR_VALUE ) return false;
  gdate_ = gx;
  jdate_ = jx;
  mdate_ = mx;
  cjdn_  = x;
  return true;
}

bool Date::impl::reset(const INT& new_cjdn)
{
  if(new_cjdn == EMPTY_CJDN) {
    cjdn_  = EMPTY_CJDN ;
    gdate_ = std::make_tuple<Year,Month,Day>({},{},{});
    jdate_ = std::make_tuple<Year,Month,Day>({},{},{});
    mdate_ = std::make_tuple<Year,Month,Day>({},{},{});
  } else {
    if(new_cjdn < MIN_CJDN_VALUE) return false;
    auto jx = cjdn2julian(new_cjdn);
    INT jy ( std::get<0>(jx) );
    if( jy < MIN_YEAR_VALUE ) return false;
    auto gx = cjdn2grigorian(new_cjdn);
    INT gy ( std::get<0>(gx) );
    if( gy < MIN_YEAR_VALUE ) return false;
    auto mx = cjdn2milankovic(new_cjdn);
    INT my ( std::get<0>(mx) );
    if( my < MIN_YEAR_VALUE ) return false;
    gdate_ = gx;
    jdate_ = jx;
    mdate_ = mx;
    cjdn_ = new_cjdn;
  }
  return true;
}

Date::impl::impl()
{
  reset();
}

Date::impl::impl(const Year& y, const Month m, const Day d, const CalendarFormat f)
{
  if(!reset(y, m, d, f))
    throw std::runtime_error(std::string(invalid_date)+" '"+y+'.'+std::to_string(m)+'.'+std::to_string(d)+'\'');
}

Date::impl::impl(const INT& cjdn)
{
  if(!reset(cjdn))
    throw std::runtime_error(std::string(invalid_date)+" : cjdn = "+cjdn.str());
}

int Date::impl::fdiv_(int a, int b) const
{//floor division
  return (a - (a < 0 ? b - 1 : 0)) / b;
}

INT Date::impl::fdiv_(const INT& a, const INT& b) const
{//floor division
  if(a==0) return 0;
  INT quotient, remainder;
  boost::multiprecision::divide_qr(a, b, quotient, remainder);
  if(remainder==0) return quotient;
  if(quotient==0 && remainder<0) return -1;
  if(quotient==0 && remainder>0) return 0;
  if(quotient<0) return quotient-1;
  else return quotient;
}

template<typename Integer>
  Integer Date::impl::mod_(const Integer& a, const Integer& b) const
{
  return a - fdiv_(a, b) * b;
}

std::pair<int,int> Date::impl::pdiv_(int a, int b) const
{//positive remainder division
  std::div_t rv = std::div(a, b);
  if(rv.rem < 0) {
      if(b>0) {
          rv.quot -= 1;
          rv.rem += b;
      } else {
          rv.quot += 1;
          rv.rem -= b;
      }
  }
  return {rv.quot, rv.rem};
}

std::pair<INT,INT> Date::impl::pdiv_(const INT& a, const INT& b) const
{//positive remainder division
  INT quotient, remainder;
  boost::multiprecision::divide_qr(a, b, quotient, remainder);
  if(remainder < 0) {
      if(b>0) {
          quotient -= 1;
          remainder += b;
      } else {
          quotient += 1;
          remainder -= b;
      }
  }
  return {quotient, remainder};
}

INT Date::impl::grigorian2cjdn(const Year& y, const Month m, const Day d) const
// Dr Louis Strous's method:
// https://aa.quae.nl/en/reken/juliaansedag.html#3_1
{
  INT year = string_to_big_int(y);
  int c0 = fdiv_((m - 3) , 12);
  INT x1 = INT(m) - INT(12) * INT(c0) - INT(3);
  INT x4 = year + c0;
  auto [x3, x2] = pdiv_(x4, INT(100));
  INT result = d + 1721119;
  result += fdiv_( INT(146097) * x3, INT(4) ) ;
  result += fdiv_( INT(36525) * x2, INT(100) ) ;
  result += fdiv_( INT(153) * x1 + INT(2), INT(5) ) ;
  return result;
}

INT Date::impl::julian2cjdn(const Year& y, const Month m, const Day d) const
// Dr Louis Strous's method:
// https://aa.quae.nl/en/reken/juliaansedag.html#5_1
{
  INT year = string_to_big_int(y);
  int c0 = fdiv_((m - 3) , 12);
  INT j1 = fdiv_(INT(1461) * (year + INT(c0)), INT(4));
  int j2 = fdiv_(153 * m - 1836 * c0 - 457, 5);
  INT result = j1 + j2 + d + 1721117;
  return result;
}

INT Date::impl::milankovic2cjdn(const Year& y, const Month m, const Day d) const
// Dr Louis Strous's method:
// https://aa.quae.nl/en/reken/juliaansedag.html#4_1
{
  INT year = string_to_big_int(y);
  int c0 = fdiv_((m - 3) , 12);
  INT x4 = year + c0;
  INT x3 = fdiv_(x4, INT(100));
  int x2 = static_cast<int>(mod_(x4, INT(100)));
  int x1 = m - c0*12 - 3;
  INT result = d + 1721119;
  result += fdiv_( INT(328718) * x3 + INT(6), INT(9) ) ;
  result += fdiv_( INT(36525) * x2, INT(100) ) ;
  result += fdiv_( 153 * x1 + 2, 5 ) ;
  return result;
}

std::tuple<Year,Month,Day> Date::impl::cjdn2grigorian(const INT& cjdn) const
// Dr Louis Strous's method:
// https://aa.quae.nl/en/reken/juliaansedag.html#3_2
{
  auto [x3, r3] = pdiv_( INT(4) * cjdn - INT(6884477), INT(146097) ) ;
  auto [x2, r2] = pdiv_( 100 * fdiv_(static_cast<int>(r3), 4) + 99, 36525 ) ;
  auto [x1, r1] = pdiv_( 5 * fdiv_(r2, 100) + 2, 153 ) ;
  int c0 = fdiv_(x1 + 2, 12);
  Day d = fdiv_(r1, 5) + 1;
  Month m = x1 - 12 * c0 + 3;
  INT y = x3*100 + x2 + c0;
  return std::make_tuple(y.str(), m, d);
}

std::tuple<Year,Month,Day> Date::impl::cjdn2julian(const INT& cjdn) const
// Dr Louis Strous's method:
// https://aa.quae.nl/en/reken/juliaansedag.html#5_2
{
  INT y2 = cjdn - 1721118;
  INT k2 = y2*4 + 3;
  int k1 = 5 * fdiv_(static_cast<int>(mod_(k2, INT(1461))), 4) + 2;
  int x1 = fdiv_(k1, 153);
  int c0 = fdiv_(x1 + 2, 12);
  INT y = fdiv_(k2, INT(1461)) + c0;
  Month m = x1 - 12 * c0 + 3;
  Day d = fdiv_(mod_(k1, 153), 5) + 1;
  return std::make_tuple(y.str(), m, d);
}

std::tuple<Year,Month,Day> Date::impl::cjdn2milankovic(const INT& cjdn) const
// Dr Louis Strous's method:
// https://aa.quae.nl/en/reken/juliaansedag.html#4_2
{
  INT k3 = INT(9) * (cjdn - INT(1721120)) + 2;
  INT x3 = fdiv_(k3, INT(328718));
  int k2 = 100 * fdiv_(static_cast<int>(mod_(k3, INT(328718))), 9) + 99;
  int x2 = fdiv_(k2, 36525);
  int k1 = fdiv_(mod_(k2, 36525), 100) * 5 + 2;
  int x1 = fdiv_(k1, 153);
  int c0 = fdiv_(x1 + 2, 12);
  INT y = x3*100 + x2 + c0;
  Month m = x1 - 12 * c0 + 3;
  Day d = fdiv_(mod_(k1, 153), 5) + 1;
  return std::make_tuple(y.str(), m, d);
}

bool Date::impl::operator==(const Date::impl& rhs) const
{
  return cjdn_==rhs.cjdn_;
}

bool Date::impl::operator!=(const Date::impl& rhs) const
{
  return !(*this==rhs);
}

bool Date::impl::operator<(const Date::impl& rhs) const
{
  return cjdn_<rhs.cjdn_;
}

bool Date::impl::operator<=(const Date::impl& rhs) const
{
  return cjdn_<=rhs.cjdn_;
}

bool Date::impl::operator>(const Date::impl& rhs) const
{
  return cjdn_>rhs.cjdn_;
}

bool Date::impl::operator>=(const Date::impl& rhs) const
{
  return cjdn_>=rhs.cjdn_;
}

bool Date::impl::is_valid() const
{
  return cjdn_ != EMPTY_CJDN;
}

Year Date::impl::year(const CalendarFormat fmt) const
{
  Year result {};
  switch(fmt){
    case Grigorian: {
      result = std::get<0>(gdate_);
    } break;
    case Julian: {
      result = std::get<0>(jdate_);
    } break;
    case Milankovic: {
      result = std::get<0>(mdate_);
    } break;
    default: {}
  }
  return result;
}

Month Date::impl::month(const CalendarFormat fmt) const
{
  Month result {};
  switch(fmt){
    case Grigorian: {
      result = std::get<1>(gdate_);
    } break;
    case Julian: {
      result = std::get<1>(jdate_);
    } break;
    case Milankovic: {
      result = std::get<1>(mdate_);
    } break;
    default: {}
  }
  return result;
}

Day Date::impl::day(const CalendarFormat fmt) const
{
  Day result {};
  switch(fmt){
    case Grigorian: {
      result = std::get<2>(gdate_);
    } break;
    case Julian: {
      result = std::get<2>(jdate_);
    } break;
    case Milankovic: {
      result = std::get<2>(mdate_);
    } break;
    default: {}
  }
  return result;
}

Weekday Date::impl::weekday() const
{
  if(!is_valid()) return -1;
  return boost::multiprecision::integer_modulus(cjdn_ + 1, 7);
}

std::tuple<Year,Month,Day> Date::impl::ymd(const CalendarFormat fmt) const
{
  switch(fmt) {
    case Grigorian:  return gdate_ ;
    case Julian:     return jdate_ ;
    case Milankovic: return mdate_ ;
  }
  return std::make_tuple<Year,Month,Day>({},{},{}) ;
}

INT Date::impl::cjdn() const
{
  return cjdn_ ;
}

INT Date::impl::cjdn_from_incremented_by(unsigned long long c) const
{
  return cjdn_ + c;
}

INT Date::impl::cjdn_from_decremented_by(unsigned long long c) const
{
  return cjdn_ - c;
}

std::string& Date::impl::format(std::string& fmt) const
{
  if(fmt.size() < 3) return fmt;
  std::string gy_ = std::get<0>(gdate_);
  std::string gm_ = std::to_string(std::get<1>(gdate_));
  std::string gd_ = std::to_string(std::get<2>(gdate_));
  std::string jy_ = std::get<0>(jdate_);
  std::string jm_ = std::to_string(std::get<1>(jdate_));
  std::string jd_ = std::to_string(std::get<2>(jdate_));
  std::string my_ = std::get<0>(mdate_);
  std::string mm_ = std::to_string(std::get<1>(mdate_));
  std::string md_ = std::to_string(std::get<2>(mdate_));
  auto replacement = [this, &jy_, &gy_, &my_, &jm_, &gm_, &mm_, &jd_, &gd_, &md_](const std::string& c)->std::string{
    if(c=="%%")      { return "%"; }
    else if(c=="JY") { return jy_; }
    else if(c=="GY") { return gy_; }
    else if(c=="MY") { return my_; }
    else if(c=="Jq") { return jm_; }
    else if(c=="Gq") { return gm_; }
    else if(c=="Mq") { return mm_; }
    else if(c=="Jd") { return jd_; }
    else if(c=="Gd") { return gd_; }
    else if(c=="Md") { return md_; }
    else if(c=="Jy") { return jy_.size()<3 ? jy_ : jy_.substr(jy_.size()-2); }
    else if(c=="Gy") { return gy_.size()<3 ? gy_ : gy_.substr(gy_.size()-2); }
    else if(c=="My") { return my_.size()<3 ? my_ : my_.substr(my_.size()-2); }
    else if(c=="JM") { return jm_.empty() ? jm_ : Date::month_name(std::stoi(jm_)); }
    else if(c=="GM") { return gm_.empty() ? gm_ : Date::month_name(std::stoi(gm_)); }
    else if(c=="MM") { return mm_.empty() ? mm_ : Date::month_name(std::stoi(mm_)); }
    else if(c=="Jm") { return jm_.empty() ? jm_ : Date::month_short_name(std::stoi(jm_)); }
    else if(c=="Gm") { return gm_.empty() ? gm_ : Date::month_short_name(std::stoi(gm_)); }
    else if(c=="Mm") { return mm_.empty() ? mm_ : Date::month_short_name(std::stoi(mm_)); }
    else if(c=="JF") { return jm_.empty() ? jm_ : Date::month_name(std::stoi(jm_), false); }
    else if(c=="GF") { return gm_.empty() ? gm_ : Date::month_name(std::stoi(gm_), false); }
    else if(c=="MF") { return mm_.empty() ? mm_ : Date::month_name(std::stoi(mm_), false); }
    else if(c=="JQ") { return jm_.size()==1 ? ('0' + jm_) : jm_ ; }
    else if(c=="GQ") { return gm_.size()==1 ? ('0' + gm_) : gm_ ; }
    else if(c=="MQ") { return mm_.size()==1 ? ('0' + mm_) : mm_ ; }
    else if(c=="JD") { return jd_.size()==1 ? ('0' + jd_) : jd_ ; }
    else if(c=="GD") { return gd_.size()==1 ? ('0' + gd_) : gd_ ; }
    else if(c=="MD") { return md_.size()==1 ? ('0' + md_) : md_ ; }
    else if(c=="wd") { return std::to_string(weekday()); }
    else if(c=="WD") { return Date::weekday_name(weekday()); }
    else if(c=="Wd") { return Date::weekday_short_name(weekday()); }
    return '%' + c;
  };
  for(std::string::size_type pos{}; (pos=fmt.find('%', pos)) != fmt.npos; ) {
    if(pos == fmt.size()-1 || pos == fmt.size()-2) return fmt;
    auto repl = replacement(fmt.substr(pos+1, 2));
    fmt.replace(pos, 3, repl);
    pos += repl.size();
  }
  return fmt;
}

/*----------------------------------------------*/
/*                  class Date                  */
/*----------------------------------------------*/

/*static*/std::string Date::month_name(Month m, bool rp)
{
  if(rp) {
    switch(m) {
      case 1: { return "Января"; }
      case 2: { return "Февраля"; }
      case 3: { return "Марта"; }
      case 4: { return "Апреля"; }
      case 5: { return "Мая"; }
      case 6: { return "Июня"; }
      case 7: { return "Июля"; }
      case 8: { return "Августа"; }
      case 9: { return "Сентября"; }
      case 10:{ return "Октября"; }
      case 11:{ return "Ноября"; }
      case 12:{ return "Декабря"; }
    };
  } else {
    switch(m) {
      case 1: { return "Январь"; }
      case 2: { return "Февраль"; }
      case 3: { return "Март"; }
      case 4: { return "Апрель"; }
      case 5: { return "Май"; }
      case 6: { return "Июнь"; }
      case 7: { return "Июль"; }
      case 8: { return "Август"; }
      case 9: { return "Сентябрь"; }
      case 10:{ return "Октябрь"; }
      case 11:{ return "Ноябрь"; }
      case 12:{ return "Декабрь"; }
    };
  }
  return {};
}

/*static*/std::string Date::month_short_name(Month m)
{
  switch(m) {
    case 1: { return "янв"; }
    case 2: { return "фев"; }
    case 3: { return "мар"; }
    case 4: { return "апр"; }
    case 5: { return "мая"; }
    case 6: { return "июн"; }
    case 7: { return "июл"; }
    case 8: { return "авг"; }
    case 9: { return "сен"; }
    case 10:{ return "окт"; }
    case 11:{ return "ноя"; }
    case 12:{ return "дек"; }
  };
  return {};
}

/*static*/std::string Date::weekday_name(Weekday w)
{
  switch(w) {
    case 0: { return "Воскресенье"; }
    case 1: { return "Понедельник"; }
    case 2: { return "Вторник"; }
    case 3: { return "Среда"; }
    case 4: { return "Четверг"; }
    case 5: { return "Пятница"; }
    case 6: { return "Суббота"; }
  };
  return {};
}

/*static*/std::string Date::weekday_short_name(Weekday w)
{
  switch(w) {
    case 0: { return "Вс"; }
    case 1: { return "Пн"; }
    case 2: { return "Вт"; }
    case 3: { return "Ср"; }
    case 4: { return "Чт"; }
    case 5: { return "Пт"; }
    case 6: { return "Сб"; }
  };
  return {};
}

/*static*/bool Date::check(const Year& y, const Month m, const Day d, const CalendarFormat fmt)
{
  try
  {
    auto p = std::make_unique<Date::impl>(y, m, d, fmt) ;
  }
  catch(const std::exception& e)
  {
    return false;
  }
  return true;
}

/*static*/bool Date::check(const unsigned long long y, const Month m, const Day d, const CalendarFormat fmt)
{
  return check(std::to_string(y), m, d, fmt);
}

Date::Date() : pimpl(new Date::impl())
{
}

Date::Date(const Year& y, const Month m, const Day d, const CalendarFormat fmt)
  : pimpl(new Date::impl(y, m, d, fmt))
{
}

Date::Date(const unsigned long long y, const Month m, const Day d, const CalendarFormat fmt)
  : pimpl(new Date::impl(std::to_string(y), m, d, fmt))
{
}

Date::Date(const Date& other) : pimpl(new Date::impl(*other.pimpl))
{
}

Date& Date::operator=(const Date& other)
{
  if(this != &other) pimpl.reset(new Date::impl(*other.pimpl));
  return *this;
}

Date::Date(Date&&) noexcept = default ;

Date& Date::operator=(Date&&) noexcept = default ;

Date::~Date() = default;

bool Date::operator==(const Date& rhs) const
{
  return *pimpl == *rhs.pimpl;
}

bool Date::operator!=(const Date& rhs) const
{
  return *pimpl != *rhs.pimpl;
}

bool Date::operator<(const Date& rhs) const
{
  return *pimpl < *rhs.pimpl;
}

bool Date::operator<=(const Date& rhs) const
{
  return *pimpl <= *rhs.pimpl;
}

bool Date::operator>(const Date& rhs) const
{
  return *pimpl > *rhs.pimpl;
}

bool Date::operator>=(const Date& rhs) const
{
  return *pimpl >= *rhs.pimpl;
}

bool Date::empty() const
{
  return !pimpl->is_valid();
}

bool Date::is_valid() const
{
  return pimpl->is_valid();
}

Date::operator bool() const
{
  return pimpl->is_valid();
}

Year Date::year(const CalendarFormat fmt) const
{
  return pimpl->year(fmt);
}

Month Date::month(const CalendarFormat fmt) const
{
  return pimpl->month(fmt);
}

Day Date::day(const CalendarFormat fmt) const
{
  return pimpl->day(fmt);
}

Weekday Date::weekday() const
{
  return pimpl->weekday();
}

std::tuple<Year,Month,Day> Date::ymd(const CalendarFormat fmt) const
{
  return pimpl->ymd(fmt);
}

Date Date::inc_by_days(unsigned long long c) const
{
  try
  {
    auto new_pimpl = std::make_unique<Date::impl>(pimpl->cjdn_from_incremented_by(c)) ;
    Date result;
    result.pimpl.swap(new_pimpl);
    return result;
  }
  catch(const std::exception& e)
  {
    return {};
  }
}

Date Date::dec_by_days(unsigned long long c) const
{
  try
  {
    auto new_pimpl = std::make_unique<Date::impl>(pimpl->cjdn_from_decremented_by(c)) ;
    Date result;
    result.pimpl.swap(new_pimpl);
    return result;
  }
  catch(const std::exception& e)
  {
    return {};
  }
}

bool Date::reset(const Year& y, const Month m, const Day d, const CalendarFormat fmt)
{
  if(pimpl->reset(y, m, d, fmt)) return true;
  else return false;
}

bool Date::reset(const unsigned long long y, const Month m, const Day d, const CalendarFormat fmt)
{
  return reset(std::to_string(y), m, d, fmt);
}

std::string Date::format(std::string fmt) const
{
  return pimpl->format(fmt);
}

/*----------------------------------------------*/
/*              class OrthYear                  */
/*----------------------------------------------*/

class OrthYear {

  ShortDate pasha_calc(const big_int& year)
  { //use Gauss method for julian calendar
    int8_t m_=3, p;
    unsigned a, b, c, d, e;
    a = static_cast<unsigned>(year % 19);
    b = static_cast<unsigned>(year % 4);
    c = static_cast<unsigned>(year % 7);
    d = (19*a+15) % 30;
    e = (2*b+4*c+6*d+6) % 7;
    p = 22 + d + e;
    if(p>31) {
      p = d + e - 9;
      m_ = 4;
    }
    return std::make_pair(m_, p);
  }

  struct Data1 {
    int8_t dn{-1};
    int8_t glas{-1};
    int8_t n50{-1};
    int8_t day{};
    int8_t month{};
    ApEvReads apostol;
    ApEvReads evangelie;
    std::array<uint16_t, M_COUNT> day_markers{};//sorted array
    bool operator<(const Data1& rhs) const
    {
      return ShortDate{month, day} < ShortDate{rhs.month, rhs.day};
    }
    bool operator==(const Data1& rhs) const = default;
    bool operator<(const ShortDate& rhs) const
    {
      return ShortDate{month, day} < rhs;
    }
    bool operator==(const ShortDate& rhs) const
    {
      return ShortDate{month, day} == rhs;
    }
  };

  struct Data2 {
    uint16_t marker{};
    int8_t day{};
    int8_t month{};
    bool operator<(const Data2& rhs) const
    {
      return marker < rhs.marker;
    }
    bool operator==(const Data2& rhs) const = default;
    bool operator<(oxc_const rhs) const
    {
      return marker < rhs;
    }
    bool operator==(oxc_const rhs) const
    {
      return marker == rhs;
    }
    friend bool operator<(oxc_const lhs, const Data2& rhs)
    {
      return lhs < rhs.marker;
    }
  };

  std::vector<Data1> data1;//sorted array
  std::vector<Data2> data2;//sorted array
  int8_t winter_indent;
  int8_t spring_indent;
  big_int y;

  std::optional<decltype(data1)::const_iterator> find_in_data1(int8_t m, int8_t d) const
  {
    auto dd = ShortDate{m, d};
    auto fr = std::lower_bound(data1.begin(), data1.end(), dd);
    if(fr==data1.end()) return std::nullopt;
    if( !(*fr==dd) ) return std::nullopt;
    return fr;
  }

public:

  OrthYear(const std::string& year, std::span<const uint8_t> il, bool osen_otstupka_apostol);
  OrthYear(const std::string& year, bool o)
    : OrthYear(year, std::array<uint8_t,17>{33,32,33,31,32,33,30,31,32,33,30,31,17,32,33,10,11}, o) {}
  OrthYear(const std::string& year): OrthYear(year, false) {}
  OrthYear(const std::string& year, std::span<const uint8_t> il): OrthYear(year, il, false) {}

  int8_t get_winter_indent() const { return winter_indent; }
  int8_t get_spring_indent() const { return spring_indent; }
  int8_t get_date_glas(int8_t month, int8_t day) const;
  int8_t get_date_n50(int8_t month, int8_t day) const;
  int8_t get_date_dn(int8_t month, int8_t day) const;
  ApEvReads get_date_apostol(int8_t month, int8_t day) const;
  ApEvReads get_date_evangelie(int8_t month, int8_t day) const;
  ApEvReads get_resurrect_evangelie(int8_t month, int8_t day) const;
  std::optional<std::vector<uint16_t>> get_date_properties(int8_t month, int8_t day) const;
  std::optional<ShortDate> get_date_with(oxc_const m) const;
  std::optional<std::vector<ShortDate>> get_alldates_with(oxc_const m) const;
  std::optional<ShortDate> get_date_withanyof(std::span<oxc_const> m) const;
  std::optional<ShortDate> get_date_withallof(std::span<oxc_const> m) const;
  std::optional<std::vector<ShortDate>> get_alldates_withanyof(std::span<oxc_const> m) const;
};

OrthYear::OrthYear(const std::string& year, std::span<const uint8_t> il, bool osen_otstupka_apostol)
{ //main constructor
  y = string_to_year(year) ;
  bool bad_il{};
  for(auto j: il) if(j<1 || j>33) bad_il = true;
  if(il.size()!=17 || bad_il)
    throw std::runtime_error("установлены некорректные параметры отступки/преступки апостольских/евангельских чтений");
  //таблица - непереходящие даты года
  static const std::array stable_dates  = {
    (int)m1d1, 1, 1,
    (int)m1d2, 1, 2,
    (int)m1d3, 1, 3,
    (int)m1d4, 1, 4,
    (int)m1d5, 1, 5,
    (int)m1d6, 1, 6,
    (int)m1d7, 1, 7,
    (int)m1d8, 1, 8,
    (int)m1d9, 1, 9,
    (int)m1d10, 1, 10,
    (int)m1d11, 1, 11,
    (int)m1d12, 1, 12,
    (int)m1d13, 1, 13,
    (int)m1d14, 1, 14,
    (int)m3d25, 3, 25,
    (int)m6d24, 6, 24,
    (int)m6d25, 6, 25,
    (int)m6d29, 6, 29,
    (int)m8d5, 8, 5,
    (int)m8d6, 8, 6,
    (int)m8d7, 8, 7,
    (int)m8d8, 8, 8,
    (int)m8d9, 8, 9,
    (int)m8d10, 8, 10,
    (int)m8d11, 8, 11,
    (int)m8d12, 8, 12,
    (int)m8d13, 8, 13,
    (int)m8d14, 8, 14,
    (int)m8d15, 8, 15,
    (int)m8d16, 8, 16,
    (int)m8d17, 8, 17,
    (int)m8d18, 8, 18,
    (int)m8d19, 8, 19,
    (int)m8d20, 8, 20,
    (int)m8d21, 8, 21,
    (int)m8d22, 8, 22,
    (int)m8d23, 8, 23,
    (int)m9d7, 9, 7,
    (int)m9d8, 9, 8,
    (int)m9d9, 9, 9,
    (int)m9d10, 9, 10,
    (int)m9d11, 9, 11,
    (int)m9d12, 9, 12,
    (int)m9d13, 9, 13,
    (int)m9d14, 9, 14,
    (int)m9d15, 9, 15,
    (int)m9d16, 9, 16,
    (int)m9d17, 9, 17,
    (int)m9d18, 9, 18,
    (int)m9d19, 9, 19,
    (int)m9d20, 9, 20,
    (int)m9d21, 9, 21,
    (int)m8d29, 8, 29,
    (int)m10d1, 10, 1,
    (int)m11d20, 11, 20,
    (int)m11d21, 11, 21,
    (int)m11d22, 11, 22,
    (int)m11d23, 11, 23,
    (int)m11d24, 11, 24,
    (int)m11d25, 11, 25,
    (int)m12d20, 12, 20,
    (int)m12d21, 12, 21,
    (int)m12d22, 12, 22,
    (int)m12d23, 12, 23,
    (int)m12d24, 12, 24,
    (int)m12d25, 12, 25,
    (int)m12d26, 12, 26,
    (int)m12d27, 12, 27,
    (int)m12d28, 12, 28,
    (int)m12d29, 12, 29,
    (int)m12d30, 12, 30,
    (int)m12d31, 12, 31
  };
  auto make_pair = [](int m, int d){ return ShortDate{m,d}; };
  //таблица - даты сплошных седмиц
  static const std::array svyatki_dates = {
    make_pair(1,1),
    make_pair(1,2),
    make_pair(1,3),
    make_pair(1,4),
    make_pair(12,25),
    make_pair(12,26),
    make_pair(12,27),
    make_pair(12,28),
    make_pair(12,29),
    make_pair(12,30),
    make_pair(12,31)
  };
  //type alias for const tables
  using TT1 = std::array<std::array<ApEvReads, 7>, 37>;
  using TT2 = std::map<uint16_t, ApEvReads>;
  //таблица рядовых чтений на литургии из приложения богосл.евангелия. период от св. троицы до нед. сыропустная
  //двумерный массив [a][b], где а - календарный номер по пятидесятнице. b - деньнедели.
  static const TT1 evangelie_table_1 {
    std::array { ApEvReads{ 0X1B5, "Ин., 27 зач., VII, 37–52; VIII, 12."},  //неделя 0. день св. троицы
            ApEvReads{},
            ApEvReads{},
            ApEvReads{},
            ApEvReads{},
            ApEvReads{},
            ApEvReads{}
          },
    std::array { ApEvReads{ 0X262, "Мф., 38 зач., X, 32–33, 37–38; XIX, 27–30."},  //Неделя 1 всех святых
            ApEvReads{ 0X4B2, "Мф., 75 зач., XVIII, 10–20."},  //пн - Святаго Духа
            ApEvReads{ 0XA2, "Мф., 10 зач., IV, 25 – V, 12."},  //вт - седмица 1
            ApEvReads{ 0XC2, "Мф., 12 зач., V, 20–26."},  //ср
            ApEvReads{ 0XD2, "Мф., 13 зач., V, 27–32."},  //чт
            ApEvReads{ 0XE2, "Мф., 14 зач., V, 33–41."},  //пт
            ApEvReads{ 0XF2, "Мф., 15 зач., V, 42–48."}  //сб
          },
    std::array { ApEvReads{ 0X92, "Мф., 9 зач., IV, 18–23."},  //Неделя 2
            ApEvReads{ 0X132, "Мф., 19 зач., VI, 31–34; VII, 9–11."},  //пн - седмица 2
            ApEvReads{ 0X162, "Мф., 22 зач., VII, 15–21."},  //вт
            ApEvReads{ 0X172, "Мф., 23 зач., VII, 21–23."},  //ср
            ApEvReads{ 0X1B2, "Мф., 27 зач., VIII, 23–27."},  //чт
            ApEvReads{ 0X1F2, "Мф., 31 зач., IX, 14–17."},  //пт
            ApEvReads{ 0X142, "Мф., 20 зач., VII, 1–8."}  //сб
          },
    std::array { ApEvReads{ 0X122, "Мф., 18 зач., VI, 22–33."},  //Неделя 3
            ApEvReads{ 0X222, "Мф., 34 зач., IX, 36 – X, 8."},  //пн - седмица 3
            ApEvReads{ 0X232, "Мф., 35 зач., X, 9–15."},  //вт
            ApEvReads{ 0X242, "Мф., 36 зач., X, 16–22."},  //ср
            ApEvReads{ 0X252, "Мф., 37 зач., X, 23–31."},  //чт
            ApEvReads{ 0X262, "Мф., 38 зач., X, 32–36; XI, 1."},  //пт
            ApEvReads{ 0X182, "Мф., 24 зач., VII, 24 – VIII, 4."}  //сб
          },
    std::array { ApEvReads{ 0X192, "Мф., 25 зач., VIII, 5–13."},  //Неделя 4
            ApEvReads{ 0X282, "Мф., 40 зач., XI, 2–15."},  //пн - седмица 4
            ApEvReads{ 0X292, "Мф., 41 зач., XI, 16–20."},  //вт
            ApEvReads{ 0X2A2, "Мф., 42 зач., XI, 20–26."},  //ср
            ApEvReads{ 0X2B2, "Мф., 43 зач., XI, 27–30."},  //чт
            ApEvReads{ 0X2C2, "Мф., 44 зач., XII, 1–8."},  //пт
            ApEvReads{ 0X1A2, "Мф., 26 зач., VIII, 14–23."}  //сб
          },
    std::array { ApEvReads{ 0X1C2, "Мф., 28 зач., VIII, 28 - IX, 1."},  //Неделя 5
            ApEvReads{ 0X2D2, "Мф., 45 зач., XII, 9-13."},  //пн - седмица 5
            ApEvReads{ 0X2E2, "Мф., 46 зач., XII, 14–16, 22–30."},  //вт
            ApEvReads{ 0X302, "Мф., 48 зач., XII, 38–45."},  //ср
            ApEvReads{ 0X312, "Мф., 49 зач., XII, 46 – XIII, 3."},  //чт
            ApEvReads{ 0X322, "Мф., 50 зач., XIII, 3–9."},  //пт
            ApEvReads{ 0X1E2, "Мф., 30 зач., IX, 9–13."}  //сб
          },
    std::array { ApEvReads{ 0X1D2, "Мф., 29 зач., IX, 1–8."},  //Неделя 6
            ApEvReads{ 0X332, "Мф., 51 зач., XIII, 10–23."},  //пн - седмица 6
            ApEvReads{ 0X342, "Мф., 52 зач., XIII, 24–30."},  //вт
            ApEvReads{ 0X352, "Мф., 53 зач., XIII, 31–36."},  //ср
            ApEvReads{ 0X362, "Мф., 54 зач., XIII, 36–43."},  //чт
            ApEvReads{ 0X372, "Мф., 55 зач., XIII, 44–54."},  //пт
            ApEvReads{ 0X202, "Мф., 32 зач., IX, 18–26."}  //сб
          },
    std::array { ApEvReads{ 0X212, "Мф., 33 зач., IX, 27–35."},  //Неделя 7
            ApEvReads{ 0X382, "Мф., 56 зач., XIII, 54–58."},  //пн - седмица 7
            ApEvReads{ 0X392, "Мф., 57 зач., XIV, 1–13."},  //вт
            ApEvReads{ 0X3C2, "Мф., 60 зач., XIV, 35 – XV, 11."},  //ср
            ApEvReads{ 0X3D2, "Мф., 61 зач., XV, 12–21."},  //чт
            ApEvReads{ 0X3F2, "Мф., 63 зач., XV, 29–31."},  //пт
            ApEvReads{ 0X272, "Мф., 39 зач., X, 37 – XI, 1."}  //сб
          },
    std::array { ApEvReads{ 0X3A2, "Мф., 58 зач., XIV, 14–22."},  //Неделя 8
            ApEvReads{ 0X412, "Мф., 65 зач., XVI, 1-6."},  //пн - седмица 8
            ApEvReads{ 0X422, "Мф., 66 зач., XVI, 6-12."},  //вт
            ApEvReads{ 0X442, "Мф., 68 зач., XVI, 20–24."},  //ср
            ApEvReads{ 0X452, "Мф., 69 зач., XVI, 24–28."},  //чт
            ApEvReads{ 0X472, "Мф., 71 зач., XVII, 10-18."},  //пт
            ApEvReads{ 0X2F2, "Мф., 47 зач., XII, 30–37."}  //сб
          },
    std::array { ApEvReads{ 0X3B2, "Мф., 59 зач., XIV, 22–34."},  //Неделя 9
            ApEvReads{ 0X4A2, "Мф., 74 зач., XVIII, 1–11."},  //пн - седмица 9
            ApEvReads{ 0X4C2, "Мф., 76 зач., XVIII, 18–22; XIX, 1–2, 13–15."},  //вт
            ApEvReads{ 0X502, "Мф., 80 зач., XX, 1–16."},  //ср
            ApEvReads{ 0X512, "Мф., 81 зач., XX, 17–28."},  //чт
            ApEvReads{ 0X532, "Мф., 83 зач., XXI, 1–11, 15–17."},  //пт
            ApEvReads{ 0X402, "Мф., 64 зач., XV, 32–39."}  //сб
          },
    std::array { ApEvReads{ 0X482, "Мф., 72 зач., XVII, 14–23."},  //Неделя 10
            ApEvReads{ 0X542, "Мф., 84 зач., XXI, 18–22."},  //пн - седмица 10
            ApEvReads{ 0X552, "Мф., 85 зач., XXI, 23–27."},  //вт
            ApEvReads{ 0X562, "Мф., 86 зач., XXI, 28–32."},  //ср
            ApEvReads{ 0X582, "Мф., 88 зач., XXI, 43-46."},  //чт
            ApEvReads{ 0X5B2, "Мф., 91 зач., XXII, 23–33."},  //пт
            ApEvReads{ 0X492, "Мф., 73 зач., XVII, 24 – XVIII, 4."}  //сб
          },
    std::array { ApEvReads{ 0X4D2, "Мф., 77 зач., XVIII, 23–35."},  //Неделя 11
            ApEvReads{ 0X5E2, "Мф., 94 зач., XXIII, 13–22."},  //пн - седмица 11
            ApEvReads{ 0X5F2, "Мф., 95 зач., XXIII, 23-28."},  //вт
            ApEvReads{ 0X602, "Мф., 96 зач., XXIII, 29–39."},  //ср
            ApEvReads{ 0X632, "Мф., 99 зач., XXIV, 13–28."},  //чт
            ApEvReads{ 0X642, "Мф., 100 зач., XXIV, 27–33, 42–51."},  //пт
            ApEvReads{ 0X4E2, "Мф., 78 зач., XIX, 3–12."}  //сб
          },
    std::array { ApEvReads{ 0X4F2, "Мф., 79 зач., XIX, 16–26."},  //Неделя 12
            ApEvReads{ 0X23, "Мк., 2 зач., I, 9–15."},  //пн - седмица 12
            ApEvReads{ 0X33, "Мк., 3 зач., I, 16–22."},  //вт
            ApEvReads{ 0X43, "Мк., 4 зач., I, 23–28."},  //ср
            ApEvReads{ 0X53, "Мк., 5 зач., I, 29-35."},  //чт
            ApEvReads{ 0X93, "Мк., 9 зач., II, 18–22."},  //пт
            ApEvReads{ 0X522, "Мф., 82 зач., XX, 29–34."}  //сб
          },
    std::array { ApEvReads{ 0X572, "Мф., 87 зач., XXI, 33–42."},  //Неделя 13
            ApEvReads{ 0XB3, "Мк., 11 зач., III, 6–12."},  //пн - седмица 13
            ApEvReads{ 0XC3, "Мк., 12 зач., III, 13–19."},  //вт
            ApEvReads{ 0XD3, "Мк., 13 зач., III, 20–27."},  //ср
            ApEvReads{ 0XE3, "Мк., 14 зач., III, 28–35."},  //чт
            ApEvReads{ 0XF3, "Мк., 15 зач., IV, 1–9."},  //пт
            ApEvReads{ 0X5A2, "Мф., 90 зач., XXII, 15-22."}  //сб
          },
    std::array { ApEvReads{ 0X592, "Мф., 89 зач., XXII, 1–14."},  //Неделя 14
            ApEvReads{ 0X103, "Мк., 16 зач., IV, 10–23."},  //пн - седмица 14
            ApEvReads{ 0X113, "Мк., 17 зач., IV, 24–34."},  //вт
            ApEvReads{ 0X123, "Мк., 18 зач., IV, 35–41."},  //ср
            ApEvReads{ 0X133, "Мк., 19 зач., V, 1-20."},  //чт
            ApEvReads{ 0X143, "Мк., 20 зач., V, 22–24, 35 – VI, 1."},  //пт
            ApEvReads{ 0X5D2, "Мф., 93 зач., XXIII, 1–12."}  //сб
          },
    std::array { ApEvReads{ 0X5C2, "Мф., 92 зач., XXII, 35–46."},  //Неделя 15
            ApEvReads{ 0X153, "Мк., 21 зач., V, 24–34."},  //пн - седмица 15
            ApEvReads{ 0X163, "Мк., 22 зач., VI, 1-7."},  //вт
            ApEvReads{ 0X173, "Мк., 23 зач., VI, 7–13."},  //ср
            ApEvReads{ 0X193, "Мк., 25 зач., VI, 30–45."},  //чт
            ApEvReads{ 0X1A3, "Мк., 26 зач., VI, 45–53."},  //пт
            ApEvReads{ 0X612, "Мф., 97 зач., XXIV, 1–13."}  //сб
          },
    std::array { ApEvReads{ 0X692, "Мф., 105 зач., XXV, 14-30."},  //Неделя 16
            ApEvReads{ 0X1B3, "Мк., 27 зач., VI, 54 - VII, 8."},  //пн - седмица 16
            ApEvReads{ 0X1C3, "Мк., 28 зач., VII, 5-16."},  //вт
            ApEvReads{ 0X1D3, "Мк., 29 зач., VII, 14–24."},  //ср
            ApEvReads{ 0X1E3, "Мк., 30 зач., VII, 24–30."},  //чт
            ApEvReads{ 0X203, "Мк., 32 зач., VIII, 1-10."},  //пт
            ApEvReads{ 0X652, "Мф., 101 зач., XXIV, 34–44."}  //сб
          },
    std::array { ApEvReads{ 0X3E2, "Мф., 62 зач., XV, 21–28."},  //Неделя 17
            ApEvReads{ 0X303, "Мк., 48 зач., X, 46–52."},  //пн - седмица 17
            ApEvReads{ 0X323, "Мк., 50 зач., XI, 11–23."},  //вт
            ApEvReads{ 0X333, "Мк., 51 зач., XI, 23–26."},  //ср
            ApEvReads{ 0X343, "Мк., 52 зач., XI, 27–33."},  //чт
            ApEvReads{ 0X353, "Мк., 53 зач., XII, 1–12."},  //пт
            ApEvReads{ 0X682, "Мф., 104 зач., XXV, 1–13."}  //сб
          },
    std::array { ApEvReads{ 0X114, "Лк., 17 зач., V, 1–11."},  //Неделя 18
            ApEvReads{ 0XA4, "Лк., 10 зач., III, 19–22."},  //пн - седмица 18
            ApEvReads{ 0XB4, "Лк., 11 зач., III, 23 – IV, 1."},  //вт
            ApEvReads{ 0XC4, "Лк., 12 зач., IV, 1-15."},  //ср
            ApEvReads{ 0XD4, "Лк., 13 зач., IV, 16–22."},  //чт
            ApEvReads{ 0XE4, "Лк., 14 зач., IV, 22–30."},  //пт
            ApEvReads{ 0XF4, "Лк., 15 зач., IV, 31–36."}  //сб
          },
    std::array { ApEvReads{ 0X1A4, "Лк., 26 зач., VI, 31–36."},  //Неделя 19
            ApEvReads{ 0X104, "Лк., 16 зач., IV, 37–44."},  //пн - седмица 19
            ApEvReads{ 0X124, "Лк., 18 зач., V, 12-16."},  //вт
            ApEvReads{ 0X154, "Лк., 21 зач., V, 33–39."},  //ср
            ApEvReads{ 0X174, "Лк., 23 зач., VI, 12–19."},  //чт
            ApEvReads{ 0X184, "Лк., 24 зач., VI, 17–23."},  //пт
            ApEvReads{ 0X134, "Лк., 19 зач., V, 17–26."}  //сб
          },
    std::array { ApEvReads{ 0X1E4, "Лк., 30 зач., VII, 11–16."},  //Неделя 20
            ApEvReads{ 0X194, "Лк., 25 зач., VI, 24–30."},  //пн - седмица 20
            ApEvReads{ 0X1B4, "Лк., 27 зач., VI, 37–45."},  //вт
            ApEvReads{ 0X1C4, "Лк., 28 зач., VI, 46 – VII, 1."},  //ср
            ApEvReads{ 0X1F4, "Лк., 31 зач., VII, 17–30."},  //чт
            ApEvReads{ 0X204, "Лк., 32 зач., VII, 31–35."},  //пт
            ApEvReads{ 0X144, "Лк., 20 зач., V, 27–32."}  //сб
          },
    std::array { ApEvReads{ 0X234, "Лк., 35 зач., VIII, 5–15."},  //Неделя 21
            ApEvReads{ 0X214, "Лк., 33 зач., VII, 36–50."},  //пн - седмица 21
            ApEvReads{ 0X224, "Лк., 34 зач., VIII, 1–3."},  //вт
            ApEvReads{ 0X254, "Лк., 37 зач., VIII, 22–25."},  //ср
            ApEvReads{ 0X294, "Лк., 41 зач., IX, 7–11."},  //чт
            ApEvReads{ 0X2A4, "Лк., 42 зач., IX, 12–18."},  //пт
            ApEvReads{ 0X164, "Лк., 22 зач., VI, 1–10."}  //сб
          },
    std::array { ApEvReads{ 0X534, "Лк., 83 зач., XVI, 19–31."},  //Неделя 22
            ApEvReads{ 0X2B4, "Лк., 43 зач., IX, 18–22."},  //пн - седмица 22
            ApEvReads{ 0X2C4, "Лк., 44 зач., IX, 23-27."},  //вт
            ApEvReads{ 0X2F4, "Лк., 47 зач., IX, 44–50."},  //ср
            ApEvReads{ 0X304, "Лк., 48 зач., IX, 49–56."},  //чт
            ApEvReads{ 0X324, "Лк., 50 зач., X, 1–15."},  //пт
            ApEvReads{ 0X1D4, "Лк., 29 зач., VII, 1–10."}  //сб
          },
    std::array { ApEvReads{ 0X264, "Лк., 38 зач., VIII, 26–39."},  //Неделя 23
            ApEvReads{ 0X344, "Лк., 52 зач., X, 22–24."},  //пн - седмица 23
            ApEvReads{ 0X374, "Лк., 55 зач., XI, 1–10."},  //вт
            ApEvReads{ 0X384, "Лк., 56 зач., XI, 9–13."},  //ср
            ApEvReads{ 0X394, "Лк., 57 зач., XI, 14–23."},  //чт
            ApEvReads{ 0X3A4, "Лк., 58 зач., XI, 23–26."},  //пт
            ApEvReads{ 0X244, "Лк., 36 зач., VIII, 16–21."}  //сб
          },
    std::array { ApEvReads{ 0X274, "Лк., 39 зач., VIII, 41–56."},  //Неделя 24
            ApEvReads{ 0X3B4, "Лк., 59 зач., XI, 29–33."},  //пн - седмица 24
            ApEvReads{ 0X3C4, "Лк., 60 зач., XI, 34–41."},  //вт
            ApEvReads{ 0X3D4, "Лк., 61 зач., XI, 42–46."},  //ср
            ApEvReads{ 0X3E4, "Лк., 62 зач., XI, 47 – XII, 1."},  //чт
            ApEvReads{ 0X3F4, "Лк., 63 зач., XII, 2–12."},  //пт
            ApEvReads{ 0X284, "Лк., 40 зач., IX, 1–6."}  //сб
          },
    std::array { ApEvReads{ 0X354, "Лк., 53 зач., X, 25–37."},  //Неделя 25
            ApEvReads{ 0X414, "Лк., 65 зач., XII, 13–15, 22–31."},  //пн - седмица 25
            ApEvReads{ 0X444, "Лк., 68 зач., XII, 42–48."},  //вт
            ApEvReads{ 0X454, "Лк., 69 зач., XII, 48-59."},  //ср
            ApEvReads{ 0X464, "Лк., 70 зач., XIII, 1–9."},  //чт
            ApEvReads{ 0X494, "Лк., 73 зач., XIII, 31–35."},  //пт
            ApEvReads{ 0X2E4, "Лк., 46 зач., IX, 37–43."}  //сб
          },
    std::array { ApEvReads{ 0X424, "Лк., 66 зач., XII, 16–21."},  //Неделя 26
            ApEvReads{ 0X4B4, "Лк., 75 зач., XIV, 12–15."},  //пн - седмица 26
            ApEvReads{ 0X4D4, "Лк., 77 зач., XIV, 25–35."},  //вт
            ApEvReads{ 0X4E4, "Лк., 78 зач., XV, 1–10."},  //ср
            ApEvReads{ 0X504, "Лк., 80 зач., XVI, 1-9."},  //чт
            ApEvReads{ 0X524, "Лк., 82 зач., XVI, 15–18; XVII, 1–4."},  //пт
            ApEvReads{ 0X314, "Лк., 49 зач., IX, 57–62."}  //сб
          },
    std::array { ApEvReads{ 0X474, "Лк., 71 зач., XIII, 10–17."},  //Неделя 27
            ApEvReads{ 0X564, "Лк., 86 зач., XVII, 20–25."},  //пн - седмица 27
            ApEvReads{ 0X574, "Лк., 87 зач., XVII, 26–37."},  //вт
            ApEvReads{ 0X5A4, "Лк., 90 зач., XVIII, 15–17, 26–30."},  //ср
            ApEvReads{ 0X5C4, "Лк., 92 зач., XVIII, 31–34."},  //чт
            ApEvReads{ 0X5F4, "Лк., 95 зач., XIX, 12–28."},  //пт
            ApEvReads{ 0X334, "Лк., 51 зач., X, 16–21."}  //сб
          },
    std::array { ApEvReads{ 0X4C4, "Лк., 76 зач., XIV, 16–24."},  //Неделя 28
            ApEvReads{ 0X614, "Лк., 97 зач., XIX, 37–44."},  //пн - седмица 28
            ApEvReads{ 0X624, "Лк., 98 зач., XIX, 45–48."},  //вт
            ApEvReads{ 0X634, "Лк., 99 зач., XX, 1–8."},  //ср
            ApEvReads{ 0X644, "Лк., 100 зач., XX, 9–18."},  //чт
            ApEvReads{ 0X654, "Лк., 101 зач., XX, 19-26."},  //пт
            ApEvReads{ 0X434, "Лк., 67 зач., XII, 32–40."}  //сб
          },
    std::array { ApEvReads{ 0X554, "Лк., 85 зач., XVII, 12–19."},  //Неделя 29
            ApEvReads{ 0X664, "Лк., 102 зач., XX, 27–44."},  //пн - седмица 29
            ApEvReads{ 0X6A4, "Лк., 106 зач., XXI, 12–19."},  //вт
            ApEvReads{ 0X684, "Лк., 104 зач., XXI, 5–7, 10–11, 20–24."},  //ср
            ApEvReads{ 0X6B4, "Лк., 107 зач., XXI, 28–33."},  //чт
            ApEvReads{ 0X6C4, "Лк., 108 зач., XXI, 37 – XXII, 8."},  //пт
            ApEvReads{ 0X484, "Лк., 72 зач., XIII, 18–29."}  //сб
          },
    std::array { ApEvReads{ 0X5B4, "Лк., 91 зач., XVIII, 18-27."},  //Неделя 30
            ApEvReads{ 0X213, "Мк., 33 зач., VIII, 11–21."},  //пн - седмица 30
            ApEvReads{ 0X223, "Мк., 34 зач., VIII, 22–26."},  //вт
            ApEvReads{ 0X243, "Мк., 36 зач., VIII, 30–34."},  //ср
            ApEvReads{ 0X273, "Мк., 39 зач., IX, 10–16."},  //чт
            ApEvReads{ 0X293, "Мк., 41 зач., IX, 33–41."},  //пт
            ApEvReads{ 0X4A4, "Лк., 74 зач., XIV, 1–11."}  //сб
          },
    std::array { ApEvReads{ 0X5D4, "Лк., 93 зач., XVIII, 35-43."},  //Неделя 31
            ApEvReads{ 0X2A3, "Мк., 42 зач., IX, 42 – X, 1."},  //пн - седмица 31
            ApEvReads{ 0X2B3, "Мк., 43 зач., X, 2–12."},  //вт
            ApEvReads{ 0X2C3, "Мк., 44 зач., X, 11–16."},  //ср
            ApEvReads{ 0X2D3, "Мк., 45 зач., X, 17–27."},  //чт
            ApEvReads{ 0X2E3, "Мк., 46 зач., X, 23–32."},  //пт
            ApEvReads{ 0X514, "Лк., 81 зач., XVI, 10–15."}  //сб
          },
    std::array { ApEvReads{ 0X5E4, "Лк., 94 зач., XIX, 1-10."},  //Неделя 32
            ApEvReads{ 0X303, "Мк., 48 зач., X, 46–52."},  //пн - седмица 32
            ApEvReads{ 0X323, "Мк., 50 зач., XI, 11–23."},  //вт
            ApEvReads{ 0X333, "Мк., 51 зач., XI, 23–26."},  //ср
            ApEvReads{ 0X343, "Мк., 52 зач., XI, 27–33."},  //чт
            ApEvReads{ 0X353, "Мк., 53 зач., XII, 1–12."},  //пт
            ApEvReads{ 0X544, "Лк., 84 зач., XVII, 3–10."}  //сб
          },
    std::array { ApEvReads{ 0X594, "Лк., 89 зач., XVIII, 10–14."},  //Неделя 33 о мытари и фарисеи
            ApEvReads{ 0X363, "Мк., 54 зач., XII, 13–17."},  //пн - седмица 33
            ApEvReads{ 0X373, "Мк., 55 зач., XII, 18–27."},  //вт
            ApEvReads{ 0X383, "Мк., 56 зач., XII, 28–37."},  //ср
            ApEvReads{ 0X393, "Мк., 57 зач., XII, 38–44."},  //чт
            ApEvReads{ 0X3A3, "Мк., 58 зач., XIII, 1–8."},  //пт
            ApEvReads{ 0X584, "Лк., 88 зач., XVIII, 2–8."}  //сб
          },
    std::array { ApEvReads{ 0X4F4, "Лк., 79 зач., XV, 11–32."},  //Неделя 34 о блуднем сыне
            ApEvReads{ 0X3B3, "Мк., 59 зач., XIII, 9–13."},  //пн - седмица 34
            ApEvReads{ 0X3C3, "Мк., 60 зач., XIII, 14-23."},  //вт
            ApEvReads{ 0X3D3, "Мк., 61 зач., XIII, 24–31."},  //ср
            ApEvReads{ 0X3E3, "Мк., 62 зач., XIII, 31 – XIV, 2."},  //чт
            ApEvReads{ 0X3F3, "Мк., 63 зач., XIV, 3-9."},  //пт
            ApEvReads{ 0X674, "Лк., 103 зач., XX, 45 – XXI, 4."}  //сб
          },
    std::array { ApEvReads{ 0X6A2, "Мф., 106 зач., XXV, 31–46."},  //Неделя 35 мясопустная
            ApEvReads{ 0X313, "Мк., 49 зач., XI, 1–11."},  //пн - седмица 35
            ApEvReads{ 0X403, "Мк., 64 зач., XIV, 10–42."},  //вт
            ApEvReads{ 0X413, "Мк., 65 зач., XIV, 43 – XV, 1."},  //ср
            ApEvReads{ 0X423, "Мк., 66 зач., XV, 1–15."},  //чт
            ApEvReads{ 0X443, "Мк., 68 зач., XV, 22, 25, 33–41."},  //пт
            ApEvReads{ 0X694, "Лк., 105 зач., XXI, 8–9, 25–27, 33–36."}  //сб
          },
    std::array { ApEvReads{ 0X112, "Мф., 17 зач., VI, 14–21."},  //Неделя 36 сыропустная
            ApEvReads{ 0X604, "Лк., 96 зач., XIX, 29–40; XXII, 7–39."},  //пн - седмица 36
            ApEvReads{ 0X6D4, "Лк., 109 зач., XXII, 39–42, 45 – XXIII, 1."},  //вт
            ApEvReads{},  //ср
            ApEvReads{ 0X6E4, "Лк., 110 зач., XXIII, 1–34, 44–56."},  //чт
            ApEvReads{},  //пт
            ApEvReads{ 0X102, "Мф., 16 зач., VI, 1–13."}  //сб
          }
  };
  auto evangelie_table1_get_chteniya = [](int8_t n50, int8_t dn) {
    return ApEvReads(evangelie_table_1.at(n50).at(dn));
  };
  //таблица рядовых чтений на литургии из приложения богосл.апостола. период от св. троицы до нед. сыропустная
  //двумерный массив [a][b], где а - календарный номер по пятидесятнице. b - деньнедели.
  static const TT1 apostol_table_1 {
    std::array { ApEvReads{ 0X31, "Деян., 3 зач., II, 1–11."},  //неделя 0. день св. троицы
            ApEvReads{},
            ApEvReads{},
            ApEvReads{},
            ApEvReads{},
            ApEvReads{},
            ApEvReads{}
          },
    std::array { ApEvReads{ 0X14A1, "Евр., 330 зач., XI, 33 – XII, 2."},  //Неделя 1 всех святых
            ApEvReads{ 0XE51, "Еф., 229 зач., V, 8–19."},  //пн - Святаго Духа
            ApEvReads{ 0X4F1, "Рим., 79 зач., I, 1–7, 13–17."},  //вт - седмица 1
            ApEvReads{ 0X501, "Рим., 80 зач., I, 18–27."},  //ср
            ApEvReads{ 0X511, "Рим., 81 зач., I, 28 – II, 9."},  //чт
            ApEvReads{ 0X521, "Рим., 82 зач., II, 14–29."},  //пт
            ApEvReads{ 0X4F1, "Рим., 79 зач., I, 7-12."}  //сб
          },
    std::array { ApEvReads{ 0X511, "Рим., 81 зач., II, 10-16."},  //Неделя 2
            ApEvReads{ 0X531, "Рим., 83 зач., II, 28 – III, 18."},  //пн - седмица 2
            ApEvReads{ 0X561, "Рим., 86 зач., IV, 4–12."},  //вт
            ApEvReads{ 0X571, "Рим., 87 зач., IV, 13–25."},  //ср
            ApEvReads{ 0X591, "Рим., 89 зач., V, 10–16."},  //чт
            ApEvReads{ 0X5A1, "Рим., 90 зач., V, 17 – VI, 2."},  //пт
            ApEvReads{ 0X541, "Рим., 84 зач., III, 19–26."}  //сб
          },
    std::array { ApEvReads{ 0X581, "Рим., 88 зач., V, 1–10."},  //Неделя 3
            ApEvReads{ 0X5E1, "Рим., 94 зач., VII, 1–13."},  //пн - седмица 3
            ApEvReads{ 0X5F1, "Рим., 95 зач., VII, 14 – VIII, 2."},  //вт
            ApEvReads{ 0X601, "Рим., 96 зач., VIII, 2–13."},//ср
            ApEvReads{ 0X621, "Рим., 98 зач., VIII, 22–27."},  //чт
            ApEvReads{ 0X651, "Рим., 101 зач., IX, 6–19."},  //пт
            ApEvReads{ 0X551, "Рим., 85 зач., III, 28 – IV, 3."}  //сб
          },
    std::array { ApEvReads{ 0X5D1, "Рим., 93 зач., VI, 18-23."},  //Неделя 4
            ApEvReads{ 0X661, "Рим., 102 зач., IX, 18–33."},  //пн - седмица 4
            ApEvReads{ 0X681, "Рим., 104 зач., X, 11 – XI, 2."},  //вт
            ApEvReads{ 0X691, "Рим., 105 зач., XI, 2–12."},  //ср
            ApEvReads{ 0X6A1, "Рим., 106 зач., XI, 13–24."},  //чт
            ApEvReads{ 0X6B1, "Рим., 107 зач., XI, 25–36."},  //пт
            ApEvReads{ 0X5C1, "Рим., 92 зач., VI, 11–17."}  //сб
          },
    std::array { ApEvReads{ 0X671, "Рим., 103 зач., X, 1–10."},  //Неделя 5
            ApEvReads{ 0X6D1, "Рим., 109 зач., XII, 4–5, 15–21."},  //пн - седмица 5
            ApEvReads{ 0X721, "Рим., 114 зач., XIV, 9–18."},  //вт
            ApEvReads{ 0X751, "Рим., 117 зач., XV, 7–16."},  //ср
            ApEvReads{ 0X761, "Рим., 118 зач., XV, 17–29."},  //чт
            ApEvReads{ 0X781, "Рим., 120 зач., XVI, 1–16."},  //пт
            ApEvReads{ 0X611, "Рим., 97 зач., VIII, 14–21."}  //сб
          },
    std::array { ApEvReads{ 0X6E1, "Рим., 110 зач., XII, 6–14."},  //Неделя 6
            ApEvReads{ 0X791, "Рим., 121 зач., XVI, 17–24."},  //пн - седмица 6
            ApEvReads{ 0X7A1, "1 Кор., 122 зач., I, 1–9."},  //вт
            ApEvReads{ 0X7F1, "1 Кор., 127 зач., II, 9 – III, 8."},  //ср
            ApEvReads{ 0X811, "1 Кор., 129 зач., III, 18–23."},  //чт
            ApEvReads{ 0X821, "1 Кор., 130 зач., IV, 5-8."},  //пт
            ApEvReads{ 0X641, "Рим., 100 зач., IX, 1–5."}  //сб
          },
    std::array { ApEvReads{ 0X741, "Рим., 116 зач., XV, 1–7."},  //Неделя 7
            ApEvReads{ 0X861, "1 Кор., 134 зач., V, 9 – VI, 11."},  //пн - седмица 7
            ApEvReads{ 0X881, "1 Кор., 136 зач., VI, 20 – VII, 12."},  //вт
            ApEvReads{ 0X891, "1 Кор., 137 зач., VII, 12–24."},  //ср
            ApEvReads{ 0X8A1, "1 Кор., 138 зач., VII, 24–35."},  //чт
            ApEvReads{ 0X8B1, "1 Кор., 139 зач., VII, 35 – VIII, 7."},  //пт
            ApEvReads{ 0X6C1, "Рим., 108 зач., XII, 1–3."}  //сб
          },
    std::array { ApEvReads{ 0X7C1, "1 Кор., 124 зач., I, 10–18."},  //Неделя 8
            ApEvReads{ 0X8E1, "1 Кор., 142 зач., IX, 13–18."},  //пн - седмица 8
            ApEvReads{ 0X901, "1 Кор., 144 зач., X, 5–12."},  //вт
            ApEvReads{ 0X911, "1 Кор., 145 зач., X, 12–22."},  //ср
            ApEvReads{ 0X931, "1 Кор., 147 зач., X, 28 – XI, 7."},  //чт
            ApEvReads{ 0X941, "1 Кор., 148 зач., XI, 8–22."},  //пт
            ApEvReads{ 0X6F1, "Рим., 111 зач., XIII, 1–10."}  //сб
          },
    std::array { ApEvReads{ 0X801, "1 Кор., 128 зач., III, 9–17."},  //Неделя 9
            ApEvReads{ 0X961, "1 Кор., 150 зач., XI, 31 – XII, 6."},  //пн - седмица 9
            ApEvReads{ 0X981, "1 Кор., 152 зач., XII, 12–26."},  //вт
            ApEvReads{ 0X9A1, "1 Кор., 154 зач., XIII, 4 – XIV, 5."},//ср
            ApEvReads{ 0X9B1, "1 Кор., 155 зач., XIV, 6–19."},  //чт
            ApEvReads{ 0X9D1, "1 Кор., 157 зач., XIV, 26–40."},  //пт
            ApEvReads{ 0X711, "Рим., 113 зач., XIV, 6–9."}  //сб
          },
    std::array { ApEvReads{ 0X831, "1 Кор., 131 зач., IV, 9–16."},  //Неделя 10
            ApEvReads{ 0X9F1, "1 Кор., 159 зач., XV, 12–19."},  //пн - седмица 10
            ApEvReads{ 0XA11, "1 Кор., 161 зач., XV, 29–38."},  //вт
            ApEvReads{ 0XA51, "1 Кор., 165 зач., XVI, 4–12."},  //ср
            ApEvReads{ 0XA71, "2 Кор., 167 зач., I, 1–7."},//чт
            ApEvReads{ 0XA91, "2 Кор., 169 зач., I, 12–20."},  //пт
            ApEvReads{ 0X771, "Рим., 119 зач., XV, 30–33."}  //сб
          },
    std::array { ApEvReads{ 0X8D1, "1 Кор., 141 зач., IX, 2–12."},  //Неделя 11
            ApEvReads{ 0XAB1, "2 Кор., 171 зач., II, 3–15."},  //пн - седмица 11
            ApEvReads{ 0XAC1, "2 Кор., 172 зач., II, 14 – III, 3."},  //вт
            ApEvReads{ 0XAD1, "2 Кор., 173 зач., III, 4–11."},  //ср
            ApEvReads{ 0XAF1, "2 Кор., 175 зач., IV, 1–6."},  //чт
            ApEvReads{ 0XB11, "2 Кор., 177 зач., IV, 13–18."},  //пт
            ApEvReads{ 0X7B1, "1 Кор., 123 зач., I, 3–9."}  //сб
          },
    std::array { ApEvReads{ 0X9E1, "1 Кор., 158 зач., XV, 1-11."},  //Неделя 12
            ApEvReads{ 0XB31, "2 Кор., 179 зач., V, 10–15."},  //пн - седмица 12
            ApEvReads{ 0XB41, "2 Кор., 180 зач., V, 15–21."},  //вт
            ApEvReads{ 0XB61, "2 Кор., 182 зач., VI, 11–16."},  //ср
            ApEvReads{ 0XB71, "2 Кор., 183 зач., VII, 1–10."},  //чт
            ApEvReads{ 0XB81, "2 Кор., 184 зач., VII, 10–16."},  //пт
            ApEvReads{ 0X7D1, "1 Кор., 125 зач., I, 18-24."}  //сб
          },
    std::array { ApEvReads{ 0XA61, "1 Кор., 166 зач., XVI, 13–24."},  //Неделя 13
            ApEvReads{ 0XBA1, "2 Кор., 186 зач., VIII, 7–15."},  //пн - седмица 13
            ApEvReads{ 0XBB1, "2 Кор., 187 зач., VIII, 16 – IX, 5."},  //вт
            ApEvReads{ 0XBD1, "2 Кор., 189 зач., IX, 12 – X, 7."},  //ср
            ApEvReads{ 0XBE1, "2 Кор., 190 зач., X, 7–18."},  //чт
            ApEvReads{ 0XC01, "2 Кор., 192 зач., XI, 5–21."},  //пт
            ApEvReads{ 0X7E1, "1 Кор., 126 зач., II, 6–9."}  //сб
          },
    std::array { ApEvReads{ 0XAA1, "2 Кор., 170 зач., I, 21 – II, 4."},  //Неделя 14
            ApEvReads{ 0XC31, "2 Кор., 195 зач., XII, 10–19."},  //пн - седмица 14
            ApEvReads{ 0XC41, "2 Кор., 196 зач., XII, 20 – XIII, 2."},  //вт
            ApEvReads{ 0XC51, "2 Кор., 197 зач., XIII, 3–13."},  //ср
            ApEvReads{ 0XC61, "Гал., 198 зач., I, 1–10, 20 – II, 5."},  //чт
            ApEvReads{ 0XC91, "Гал., 201 зач., II, 6–10."},  //пт
            ApEvReads{ 0X821, "1 Кор., 130 зач., IV, 1–5."}  //сб
          },
    std::array { ApEvReads{ 0XB01, "2 Кор., 176 зач., IV, 6–15."},  //Неделя 15
            ApEvReads{ 0XCA1, "Гал., 202 зач., II, 11–16."},  //пн - седмица 15
            ApEvReads{ 0XCC1, "Гал., 204 зач., II, 21 – III, 7."},  //вт
            ApEvReads{ 0XCF1, "Гал., 207 зач., III, 15–22."},  //ср
            ApEvReads{ 0XD01, "Гал., 208 зач., III, 23 - IV, 5."},  //чт
            ApEvReads{ 0XD21, "Гал., 210 зач., IV, 8–21."},  //пт
            ApEvReads{ 0X841, "1 Кор., 132 зач., IV, 17 – V, 5."}  //сб
          },
    std::array { ApEvReads{ 0XB51, "2 Кор., 181 зач., VI, 1–10."},  //Неделя 16
            ApEvReads{ 0XD31, "Гал., 211 зач., IV, 28 – V, 10."},  //пн - седмица 16
            ApEvReads{ 0XD41, "Гал., 212 зач., V, 11–21."},  //вт
            ApEvReads{ 0XD61, "Гал., 214 зач., VI, 2–10."},  //ср
            ApEvReads{ 0XD81, "Еф., 216 зач., I, 1–9."},  //чт
            ApEvReads{ 0XD91, "Еф., 217 зач., I, 7–17."},  //пт
            ApEvReads{ 0X921, "1 Кор., 146 зач., X, 23–28."}  //сб
          },
    std::array { ApEvReads{ 0XB61, "2 Кор., 182 зач., VI, 16 - VII, 1."},//Неделя 17
            ApEvReads{ 0XDB1, "Еф., 219 зач., I, 22 – II, 3."},  //пн - седмица 17
            ApEvReads{ 0XDE1, "Еф., 222 зач., II, 19 – III, 7."},  //вт
            ApEvReads{ 0XDF1, "Еф., 223 зач., III, 8–21."},  //ср
            ApEvReads{ 0XE11, "Еф., 225 зач., IV, 14–19."},  //чт
            ApEvReads{ 0XE21, "Еф., 226 зач., IV, 17–25."},  //пт
            ApEvReads{ 0X9C1, "1 Кор., 156 зач., XIV, 20–25."}  //сб
          },
    std::array { ApEvReads{ 0XBC1, "2 Кор., 188 зач., IX, 6–11."},  //Неделя 18
            ApEvReads{ 0XE31, "Еф., 227 зач., IV, 25–32."},  //пн - седмица 18
            ApEvReads{ 0XE61, "Еф., 230 зач., V, 20–26."},  //вт
            ApEvReads{ 0XE71, "Еф., 231 зач., V, 25–33."},  //ср
            ApEvReads{ 0XE81, "Еф., 232 зач., V, 33 – VI, 9."},  //чт
            ApEvReads{ 0XEA1, "Еф., 234 зач., VI, 18–24."},  //пт
            ApEvReads{ 0XA21, "1 Кор., 162 зач., XV, 39–45."}  //сб
          },
    std::array { ApEvReads{ 0XC21, "2 Кор., 194 зач., XI, 31 – XII, 9."},  //Неделя 19
            ApEvReads{ 0XEB1, "Флп., 235 зач., I, 1–7."},  //пн - седмица 19
            ApEvReads{ 0XEC1, "Флп., 236 зач., I, 8–14."},  //вт
            ApEvReads{ 0XED1, "Флп., 237 зач., I, 12–20."},  //ср
            ApEvReads{ 0XEE1, "Флп., 238 зач., I, 20–27."},  //чт
            ApEvReads{ 0XEF1, "Флп., 239 зач., I, 27 – II, 4."},  //пт
            ApEvReads{ 0XA41, "1 Кор., 164 зач., XV, 58 – XVI, 3."}  //сб
          },
    std::array { ApEvReads{ 0XC81, "Гал., 200 зач., I, 11–19."},  //Неделя 20
            ApEvReads{ 0XF11, "Флп., 241 зач., II, 12–16."},  //пн - седмица 20
            ApEvReads{ 0XF21, "Флп., 242 зач., II, 16–23."},  //вт
            ApEvReads{ 0XF31, "Флп., 243 зач., II, 24–30."},  //ср
            ApEvReads{ 0XF41, "Флп., 244 зач., III, 1–8."},  //чт
            ApEvReads{ 0XF51, "Флп., 245 зач., III, 8–19."},  //пт
            ApEvReads{ 0XA81, "2 Кор., 168 зач., I, 8–11."}  //сб
          },
    std::array { ApEvReads{ 0XCB1, "Гал., 203 зач., II, 16–20."},  //Неделя 21
            ApEvReads{ 0XF81, "Флп., 248 зач., IV, 10–23."},  //пн - седмица 21
            ApEvReads{ 0XF91, "Кол., 249 зач., I, 1–2, 7–11."},  //вт
            ApEvReads{ 0XFB1, "Кол., 251 зач., I, 18–23."},  //ср
            ApEvReads{ 0XFC1, "Кол., 252 зач., I, 24–29."},  //чт
            ApEvReads{ 0XFD1, "Кол., 253 зач., II, 1–7."},  //пт
            ApEvReads{ 0XAE1, "2 Кор., 174 зач., III, 12–18."}  //сб
          },
    std::array { ApEvReads{ 0XD71, "Гал., 215 зач., VI, 11–18."},  //Неделя 22
            ApEvReads{ 0XFF1, "Кол., 255 зач., II, 13–20."},  //пн - седмица 22
            ApEvReads{ 0X1001, "Кол., 256 зач., II, 20 – III, 3."},  //вт
            ApEvReads{ 0X1031, "Кол., 259 зач., III, 17 – IV, 1."},  //ср
            ApEvReads{ 0X1041, "Кол., 260 зач., IV, 2–9."},  //чт
            ApEvReads{ 0X1051, "Кол., 261 зач., IV, 10–18."},  //пт
            ApEvReads{ 0XB21, "2 Кор., 178 зач., V, 1–10."}  //сб
          },
    std::array { ApEvReads{ 0XDC1, "Еф., 220 зач., II, 4–10."},  //Неделя 23
            ApEvReads{ 0X1061, "1 Сол., 262 зач., I, 1–5."},  //пн - седмица 23
            ApEvReads{ 0X1071, "1 Сол., 263 зач., I, 6–10."},  //вт
            ApEvReads{ 0X1081, "1 Сол., 264 зач., II, 1–8."},  //ср
            ApEvReads{ 0X1091, "1 Сол., 265 зач., II, 9–14."},  //чт
            ApEvReads{ 0X10A1, "1 Сол., 266 зач., II, 14–19."},  //пт
            ApEvReads{ 0XB91, "2 Кор., 185 зач., VIII, 1–5."}  //сб
          },
    std::array { ApEvReads{ 0XDD1, "Еф., 221 зач., II, 14–22."},  //Неделя 24
            ApEvReads{ 0X10B1, "1 Сол., 267 зач., II, 20 – III, 8."},  //пн - седмица 24
            ApEvReads{ 0X10C1, "1 Сол., 268 зач., III, 9–13."},  //вт
            ApEvReads{ 0X10D1, "1 Сол., 269 зач., IV, 1–12."},  //ср
            ApEvReads{ 0X10F1, "1 Сол., 271 зач., V, 1–8."},  //чт
            ApEvReads{ 0X1101, "1 Сол., 272 зач., V, 9–13, 24–28."},  //пт
            ApEvReads{ 0XBF1, "2 Кор., 191 зач., XI, 1–6."}  //сб
          },
    std::array { ApEvReads{ 0XE01, "Еф., 224 зач., IV, 1–6."},  //Неделя 25
            ApEvReads{ 0X1121, "2 Сол., 274 зач., I, 1–10."},  //пн - седмица 25
            ApEvReads{ 0X1121, "2 Сол., 274 зач., I, 10 - II, 2."},  //вт
            ApEvReads{ 0X1131, "2 Сол., 275 зач., II, 1–12."},  //ср
            ApEvReads{ 0X1141, "2 Сол., 276 зач., II, 13 – III, 5."},  //чт
            ApEvReads{ 0X1151, "2 Сол., 277 зач., III, 6–18."},  //пт
            ApEvReads{ 0XC71, "Гал., 199 зач., I, 3–10."}  //сб
          },
    std::array { ApEvReads{ 0XE51, "Еф., 229 зач., V, 8–19."},  //Неделя 26
            ApEvReads{ 0X1161, "1 Тим., 278 зач., I, 1–7."},  //пн - седмица 26
            ApEvReads{ 0X1171, "1 Тим., 279 зач., I, 8–14."},  //вт
            ApEvReads{ 0X1191, "1 Тим., 281 зач., I, 18–20; II, 8–15."},  //ср
            ApEvReads{ 0X11B1, "1 Тим., 283 зач., III, 1–13."},  //чт
            ApEvReads{ 0X11D1, "1 Тим., 285 зач., IV, 4–8, 16."},  //пт
            ApEvReads{ 0XCD1, "Гал., 205 зач., III, 8–12."}  //сб
          },
    std::array { ApEvReads{ 0XE91, "Еф., 233 зач., VI, 10–17."},  //Неделя 27
            ApEvReads{ 0X11D1, "1 Тим., 285 зач., V, 1-10."},  //пн - седмица 27
            ApEvReads{ 0X11E1, "1 Тим., 286 зач., V, 11–21."},  //вт
            ApEvReads{ 0X11F1, "1 Тим., 287 зач., V, 22 – VI, 11."},  //ср
            ApEvReads{ 0X1211, "1 Тим., 289 зач., VI, 17–21."},  //чт
            ApEvReads{ 0X1221, "2 Тим., 290 зач., I, 1–2, 8–18."},  //пт
            ApEvReads{ 0XD51, "Гал., 213 зач., V, 22 – VI, 2."}  //сб
          },
    std::array { ApEvReads{ 0XFA1, "Кол., 250 зач., I, 12–18."},  //Неделя 28
            ApEvReads{ 0X1261, "2 Тим., 294 зач., II, 20–26."},  //пн - седмица 28
            ApEvReads{ 0X1291, "2 Тим., 297 зач., III, 16 – IV, 4."},  //вт
            ApEvReads{ 0X12B1, "2 Тим., 299 зач., IV, 9–22."},  //ср
            ApEvReads{ 0X12C1, "Тит., 300 зач., I, 5 - II, 1."},  //чт
            ApEvReads{ 0X12D1, "Тит., 301 зач., I, 15 – II, 10."},  //пт
            ApEvReads{ 0XDA1, "Еф., 218 зач., I, 16–23."}  //сб
          },
    std::array { ApEvReads{ 0X1011, "Кол., 257 зач., III, 4-11."},  //Неделя 29
            ApEvReads{ 0X1341, "Евр., 308 зач., III, 5–11, 17–19."},  //пн - седмица 29
            ApEvReads{ 0X1361, "Евр., 310 зач., IV, 1–13."},  //вт
            ApEvReads{ 0X1381, "Евр., 312 зач., V, 11 – VI, 8."},  //ср
            ApEvReads{ 0X13B1, "Евр., 315 зач., VII, 1–6."},  //чт
            ApEvReads{ 0X13D1, "Евр., 317 зач., VII, 18–25."},  //пт
            ApEvReads{ 0XDC1, "Еф., 220 зач., II, 11-13."}  //сб
          },
    std::array { ApEvReads{ 0X1021, "Кол., 258 зач., III, 12–16."},  //Неделя 30
            ApEvReads{ 0X13F1, "Евр., 319 зач., VIII, 7–13."},  //пн - седмица 30
            ApEvReads{ 0X1411, "Евр., 321 зач., IX, 8–10, 15–23."},  //вт
            ApEvReads{ 0X1431, "Евр., 323 зач., X, 1–18."},  //ср
            ApEvReads{ 0X1461, "Евр., 326 зач., X, 35 – XI, 7."},  //чт
            ApEvReads{ 0X1471, "Евр., 327 зач., XI, 8, 11–16."},  //пт
            ApEvReads{ 0XE41, "Еф., 228 зач., V, 1–8."}  //сб
          },
    std::array { ApEvReads{ 0X1181, "1 Тим., 280 зач., I, 15-17."},  //Неделя 31
            ApEvReads{ 0X1491, "Евр., 329 зач., XI, 17–23, 27–31."},//пн - седмица 31
            ApEvReads{ 0X14D1, "Евр., 333 зач., XII, 25–26; XIII, 22–25."},//вт
            ApEvReads{ 0X321, "Иак., 50 зач., I, 1-18."},  //ср
            ApEvReads{ 0X331, "Иак., 51 зач., I, 19-27."},  //чт
            ApEvReads{ 0X341, "Иак., 52 зач., II, 1–13."},  //пт
            ApEvReads{ 0XF91, "Кол., 249 зач., I, 3-6."}  //сб
          },
    std::array { ApEvReads{ 0X11D1, "1 Тим., 285 зач., IV, 9-15."},  //Неделя 32
            ApEvReads{ 0X351, "Иак., 53 зач., II, 14–26."},  //пн - седмица 32
            ApEvReads{ 0X361, "Иак., 54 зач., III, 1–10."},  //вт
            ApEvReads{ 0X371, "Иак., 55 зач., III, 11 – IV, 6."},  //ср
            ApEvReads{ 0X381, "Иак., 56 зач., IV, 7 – V, 9."},  //чт
            ApEvReads{ 0X3A1, "1 Пет., 58 зач., I, 1–2, 10–12; II, 6–10."},//пт
            ApEvReads{ 0X1111, "1 Сол., 273 зач., V, 14–23."}  //сб
          },
    std::array { ApEvReads{ 0X1281, "2 Тим., 296 зач., III, 10–15."},  //Неделя 33 о мытари и фарисеи
            ApEvReads{ 0X3B1, "1 Пет., 59 зач., II, 21 – III, 9."},  //пн - седмица 33
            ApEvReads{ 0X3C1, "1 Пет., 60 зач., III, 10–22."},  //вт
            ApEvReads{ 0X3D1, "1 Пет., 61 зач., IV, 1–11."},  //ср
            ApEvReads{ 0X3E1, "1 Пет., 62 зач., IV, 12 – V, 5."},  //чт
            ApEvReads{ 0X401, "2 Пет., 64 зач., I, 1–10."},  //пт
            ApEvReads{ 0X1251, "2 Тим., 293 зач., II, 11–19."}  //сб
          },
    std::array { ApEvReads{ 0X871, "1 Кор., 135 зач., VI, 12-20."},  //Неделя 34 о блуднем сыне
            ApEvReads{ 0X421, "2 Пет., 66 зач., I, 20 – II, 9."},  //пн - седмица 34
            ApEvReads{ 0X431, "2 Пет., 67 зач., II, 9–22."},  //вт
            ApEvReads{ 0X441, "2 Пет., 68 зач., III, 1–18."},  //ср
            ApEvReads{ 0X451, "1 Ин., 69 зач., I, 8 – II, 6."},  //чт
            ApEvReads{ 0X461, "1 Ин., 70 зач., II, 7–17."},  //пт
            ApEvReads{ 0X1271, "2 Тим., 295 зач., III, 1–9."}  //сб
          },
    std::array { ApEvReads{ 0X8C1, "1 Кор., 140 зач., VIII, 8 – IX, 2."},  //Неделя 35 мясопустная
            ApEvReads{ 0X471, "1 Ин., 71 зач., II, 18 – III, 10."},  //пн - седмица 35
            ApEvReads{ 0X481, "1 Ин., 72 зач., III, 10–20."},  //вт
            ApEvReads{ 0X491, "1 Ин., 73 зач., III, 21 – IV, 6."},  //ср
            ApEvReads{ 0X4A1, "1 Ин., 74 зач., IV, 20 – V, 21."},  //чт
            ApEvReads{ 0X4B1, "2 Ин., 75 зач., I, 1–13."},  //пт
            ApEvReads{ 0X921, "1 Кор., 146 зач., X, 23–28."}  //сб
          },
    std::array { ApEvReads{ 0X701, "Рим., 112 зач., XIII, 11 – XIV, 4."},  //Неделя 36 сыропустная
            ApEvReads{ 0X4C1, "3 Ин., 76 зач., I, 1–15."},  //пн - седмица 36
            ApEvReads{ 0X4D1, "Иуд., 77 зач., I, 1–10."},  //вт
            ApEvReads{},  //ср
            ApEvReads{ 0X4E1, "Иуд., 78 зач., I, 11–25."},  //чт
            ApEvReads{},  //пт
            ApEvReads{ 0X731, "Рим., 115 зач., XIV, 19–26."}  //сб
          }
  };
  auto apostol_table1_get_chteniya = [](int8_t n50, int8_t dn) {
    return ApEvReads(apostol_table_1.at(n50).at(dn));
  };
  //таблица рядовых чтений на литургии из приложения богосл.евангелия. период от начала вел.поста до Троицкая суб.вкл.
  //асс.массив, где first - константа-признак даты (блок 1 - переходящие дни года)
  static const TT2 evangelie_table_2 {
    {1,    { 0X15, "Ин., 1 зач., I, 1–17." } },//пасха
    {2,    { 0X25, "Ин., 2 зач., I, 18–28." } },
    {3,    { 0X714, "Лк., 113 зач., XXIV, 12–35."  } },
    {4,    { 0X45, "Ин., 4 зач., I, 35–51." } },
    {5,    { 0X85, "Ин., 8 зач., III, 1–15." } },
    {6,    { 0X75, "Ин., 7 зач., II, 12–22." } },
    {7,    { 0XB5, "Ин., 11 зач., III, 22–33." } },
    {8,    { 0X415, "Ин., 65 зач., XX, 19–31." } },//Неделя 2, о Фоме
    {9,    { 0X65, "Ин., 6 зач., II, 1–11." } },
    {10,   { 0XA5, "Ин., 10 зач., III, 16–21." } },
    {11,   { 0XF5, "Ин., 15 зач., V, 17–24." } },
    {12,   { 0X105, "Ин., 16 зач., V, 24–30." } },
    {13,   { 0X115, "Ин., 17 зач., V, 30 – VI, 2." } },
    {14,   { 0X135, "Ин., 19 зач., VI, 14–27." } },
    {15,   { 0X453, "Мк., 69 зач., XV, 43–47." } },//Неделя 3, о мироносицах
    {16,   { 0XD5, "Ин., 13 зач., IV, 46–54." } },
    {17,   { 0X145, "Ин., 20 зач., VI, 27–33." } },
    {18,   { 0X155, "Ин., 21 зач., VI, 35–39." } },
    {19,   { 0X165, "Ин., 22 зач., VI, 40–44." } },
    {20,   { 0X175, "Ин., 23 зач., VI, 48–54." } },
    {21,   { 0X345, "Ин., 52 зач., XV, 17 – XVI, 2." } },
    {22,   { 0XE5, "Ин., 14 зач., V, 1–15." } },//Неделя 4, о разслабленнем
    {23,   { 0X185, "Ин., 24 зач., VI, 56–69." } },
    {24,   { 0X195, "Ин., 25 зач., VII, 1–13." } },
    {25,   { 0X1A5, "Ин., 26 зач., VII, 14–30." } },
    {26,   { 0X1D5, "Ин., 29 зач., VIII, 12–20." } },
    {27,   { 0X1E5, "Ин., 30 зач., VIII, 21–30." } },
    {28,   { 0X1F5, "Ин., 31 зач., VIII, 31–42." } },
    {29,   { 0XC5, "Ин., 12 зач., IV, 5–42." } },//Неделя 5, о самаряныни
    {30,   { 0X205, "Ин., 32 зач., VIII, 42–51." } },
    {31,   { 0X215, "Ин., 33 зач., VIII, 51–59." } },
    {32,   { 0X125, "Ин., 18 зач., VI, 5–14." } },
    {33,   { 0X235, "Ин., 35 зач., IX, 39 – X, 9." } },
    {34,   { 0X255, "Ин., 37 зач., X, 17–28." } },
    {35,   { 0X265, "Ин., 38 зач., X, 27–38." } },
    {36,   { 0X225, "Ин., 34 зач., IX, 1–38." } },//Неделя 6, о слепом
    {37,   { 0X285, "Ин., 40 зач., XI, 47–57." } },
    {38,   { 0X2A5, "Ин., 42 зач., XII, 19–36." } },
    {39,   { 0X2B5, "Ин., 43 зач., XII, 36–47." } },
    {40,   { 0X724, "Лк., 114 зач., XXIV, 36–53."  } },//Вознесение Господне
    {41,   { 0X2F5, "Ин., 47 зач., XIV, 1–11." } },
    {42,   { 0X305, "Ин., 48 зач., XIV, 10–21." } },
    {43,   { 0X385, "Ин., 56 зач., XVII, 1–13." } },//Неделя 7, святых отец
    {44,   { 0X315, "Ин., 49 зач., XIV, 27 – XV, 7." } },
    {45,   { 0X355, "Ин., 53 зач., XVI, 2–13." } },
    {46,   { 0X365, "Ин., 54 зач., XVI, 15–23." } },
    {47,   { 0X375, "Ин., 55 зач., XVI, 23–33." } },
    {48,   { 0X395, "Ин., 57 зач., XVII, 18–26." } },
    {49,   { 0X435, "Ин., 67 зач., XXI, 15–25." } },
    {92,   { 0XA3, "Мк., 10 зач., II, 23 – III, 5." } },//Суббота первая поста
    {93,   { 0X55, "Ин., 5 зач., I, 43–51." } },//Неделя первая поста
    {99,   { 0X63, "Мк., 6 зач., I, 35–44." } },//Суббота вторая поста
    {100,  { 0X73, "Мк., 7 зач., II, 1–12." } },//Неделя вторая поста
    {106,  { 0X83, "Мк., 8 зач., II, 14–17." } },//Суббота третия поста
    {107,  { 0X253, "Мк., 37 зач., VIII, 34 – IX, 1." } },//Неделя третия поста
    {113,  { 0X1F3, "Мк., 31 зач., VII, 31–37." } },//Суббота четвертая поста
    {114,  { 0X283, "Мк., 40 зач., IX, 17–31." } },//Неделя четвертая постa
    {120,  { 0X233, "Мк., 35 зач., VIII, 27–31." } },//Суббота пятая поста
    {121,  { 0X2F3, "Мк., 47 зач., X, 32–45." } },//Неделя пятая поста
    {127,  { 0X275, "Ин., 39 зач., XI, 1–45." } },//Суббота шестая Лазарева
    {128,  { 0X295, "Ин., 41 зач., XII, 1–18." } },//В неделю цветоносную
    {129,  { 0X622, "Мф., 98 зач., XXIV, 3–35." } },//великий Понедельник
    {130,  { 0X662, "Мф., 102 зач., XXIV, 36 - XXVI, 2." } },//великий Вторник
    {131,  { 0X6C2, "Мф., 108 зач., XXVI, 6-16." } },//великую Среду
    {132,  { 0X6B2, "Мф., 107 зач., XXVI, 1–20. Ин., 44 зач., XIII, 3–17. Мф., 108 зач.(от полу́), XXVI, 21–39. Лк., 109 зач., XXII, 43–45. Мф., 108 зач., XXVI, 40 – XXVII, 2." } },//великий Четверток
    {134,  { 0X732, "Мф., 115 зач., XXVIII, 1–20." } } //великую Субботу
  };
  auto evangelie_table2_get_chteniya = [](const std::set<uint16_t>& markers)->ApEvReads {
    if(markers.empty()) return ApEvReads();
    std::vector<uint16_t> t_(evangelie_table_2.size());
    std::transform(evangelie_table_2.cbegin(), evangelie_table_2.cend(),
                    t_.begin(),
                    [](const auto& e){ return e.first; });
    auto fr1 = std::find_first_of(markers.begin(), markers.end(), t_.begin(), t_.end());
    if(fr1==markers.end()) return ApEvReads();
    if(auto fr2 = evangelie_table_2.find(*fr1); fr2 != evangelie_table_2.end()) {
      return ApEvReads(fr2->second);
    } else {
      return ApEvReads();
    }
  };
  //таблица рядовых чтений на литургии из приложения богосл.апостола. период от начала вел.поста до Троицкая суб.вкл.
  //асс.массив, где first - константа-признак даты (блок 1 - переходящие дни года)
  static const TT2 apostol_table_2 {
    {1,    { 0X11, "Деян., 1 зач., I, 1–8." } },   //пасха
    {2,    { 0X21, "Деян., 2 зач., I, 12–17, 21–26." } },
    {3,    { 0X41, "Деян., 4 зач., II, 14–21." } },
    {4,    { 0X51, "Деян., 5 зач., II, 22–36." } },
    {5,    { 0X61, "Деян., 6 зач., II, 38–43." } },
    {6,    { 0X71, "Деян., 7 зач., III, 1–8." } },
    {7,    { 0X81, "Деян., 8 зач., III, 11–16." } },
    {8,    { 0XE1, "Деян., 14 зач., V, 12–20." } },//Неделя 2, о Фоме
    {9,    { 0X91, "Деян., 9 зач., III, 19–26." } },
    {10,   { 0XA1, "Деян., 10 зач., IV, 1–10." } },
    {11,   { 0XB1, "Деян., 11 зач., IV, 13–22." } },
    {12,   { 0XC1, "Деян., 12 зач., IV, 23–31." } },
    {13,   { 0XD1, "Деян., 13 зач., V, 1–11." } },
    {14,   { 0XF1, "Деян., 15 зач., V, 21–33." } },
    {15,   { 0X101, "Деян., 16 зач., VI, 1-7." } },//Неделя 3, о мироносицах
    {16,   { 0X111, "Деян., 17 зач., VI, 8 – VII, 5, 47–60." } },
    {17,   { 0X121, "Деян., 18 зач., VIII, 5–17." } },
    {18,   { 0X131, "Деян., 19 зач., VIII, 18–25." } },
    {19,   { 0X141, "Деян., 20 зач., VIII, 26–39." } },
    {20,   { 0X151, "Деян., 21 зач., VIII, 40 – IX, 19." } },
    {21,   { 0X161, "Деян., 22 зач., IX, 19–31." } },
    {22,   { 0X171, "Деян., 23 зач., IX, 32-42." } },//Неделя 4, о разслабленнем
    {23,   { 0X181, "Деян., 24 зач., X, 1–16." } },
    {24,   { 0X191, "Деян., 25 зач., X, 21–33." } },
    {25,   { 0X221, "Деян., 34 зач., XIV, 6–18." } },
    {26,   { 0X1A1, "Деян., 26 зач., X, 34–43." } },
    {27,   { 0X1B1, "Деян., 27 зач., X, 44 – XI, 10." } },
    {28,   { 0X1D1, "Деян., 29 зач., XII, 1–11." } },
    {29,   { 0X1C1, "Деян., 28 зач., XI, 19–26, 29–30." } },//Неделя 5, о самаряныни
    {30,   { 0X1E1, "Деян., 30 зач., XII, 12–17." } },
    {31,   { 0X1F1, "Деян., 31 зач., XII, 25 – XIII, 12." } },
    {32,   { 0X201, "Деян., 32 зач., XIII, 13–24." } },
    {33,   { 0X231, "Деян., 35 зач., XIV, 20–27." } },
    {34,   { 0X241, "Деян., 36 зач., XV, 5–34." } },
    {35,   { 0X251, "Деян., 37 зач., XV, 35–41." } },
    {36,   { 0X261, "Деян., 38 зач., XVI, 16–34." } },//Неделя 6, о слепом
    {37,   { 0X271, "Деян., 39 зач., XVII, 1–15." } },
    {38,   { 0X281, "Деян., 40 зач., XVII, 19-28." } },
    {39,   { 0X291, "Деян., 41 зач., XVIII, 22–28." } },
    {40,   { 0X11, "Деян., 1 зач., I, 1–12." } },//Вознесение Господне
    {41,   { 0X2A1, "Деян., 42 зач., XIX, 1–8." } },
    {42,   { 0X2B1, "Деян., 43 зач., XX, 7–12." } },
    {43,   { 0X2C1, "Деян., 44 зач., XX, 16-18, 28-36." } },//Неделя 7, святых отец
    {44,   { 0X2D1, "Деян., 45 зач., XXI, 8–14." } },
    {45,   { 0X2E1, "Деян., 46 зач., XXI, 26–32." } },
    {46,   { 0X2F1, "Деян., 47 зач., XXIII, 1–11." } },
    {47,   { 0X301, "Деян., 48 зач., XXV, 13–19." } },
    {48,   { 0X321, "Деян., 50 зач., XXVII, 1–44." } },
    {49,   { 0X331, "Деян., 51 зач., XXVIII, 1–31." } },
    {92,   { 0X12F1, "Евр., 303 зач., I, 1–12." } },//Суббота первая поста
    {93,   { 0X1491, "Евр., 329 зач., XI, 24-26, 32 - XII, 2." } },//Неделя первая поста
    {99,   { 0X1351, "Евр., 309 зач., III, 12–16." } },//Суббота вторая поста
    {100,  { 0X1301, "Евр., 304 зач., I, 10 – II, 3." } },//Неделя вторая поста
    {106,  { 0X1451, "Евр., 325 зач., X, 32–38." } },//Суббота третия поста
    {107,  { 0X1371, "Евр., 311 зач., IV, 14 – V, 6." } },//Неделя третия поста
    {113,  { 0X1391, "Евр., 313 зач., VI, 9–12." } },//Суббота четвертая поста
    {114,  { 0X13A1, "Евр., 314 зач., VI, 13–20." } },//Неделя четвертая постa
    {120,  { 0X1421, "Евр., 322 зач., IX, 24–28." } },//Суббота пятая поста
    {121,  { 0X1411, "Евр., 321 зач., IX, 11-14." } },//Неделя пятая поста
    {127,  { 0X14D1, "Евр., 333 зач., XII, 28 - XIII, 8." } },//Суббота шестая Лазарева
    {128,  { 0XF71, "Флп., 247 зач., IV, 4-9." } },//В неделю цветоносную
    {132,  { 0X951, "1 Кор., 149 зач., XI, 23–32." } },//великий Четверток
    {134,  { 0X5B1, "Рим., 91 зач., VI, 3–11." } }//великую Субботу

  };
  auto apostol_table2_get_chteniya = [](const std::set<uint16_t>& markers)->ApEvReads {
    if(markers.empty()) return ApEvReads();
    std::vector<uint16_t> t_(apostol_table_2.size());
    std::transform(apostol_table_2.cbegin(), apostol_table_2.cend(),
                    t_.begin(),
                    [](const auto& e){ return e.first; });
    auto fr1 = std::find_first_of(markers.begin(), markers.end(), t_.begin(), t_.end());
    if(fr1==markers.end()) return ApEvReads();
    if(auto fr2 = apostol_table_2.find(*fr1); fr2 != apostol_table_2.end()) {
      return ApEvReads(fr2->second);
    } else {
      return ApEvReads();
    }
  };
  //prepare second ctor parameter
  std::array<int,5> zimn_otstupka_n5;
  std::array<int,4> zimn_otstupka_n4;
  std::array<int,3> zimn_otstupka_n3;
  std::array<int,2> zimn_otstupka_n2;
  int               zimn_otstupka_n1;
  std::array<int,2> osen_otstupka;
  auto ilit = il.begin();
  zimn_otstupka_n1 = *ilit; ++ilit;
  zimn_otstupka_n2[0] = *ilit; ++ilit;
  zimn_otstupka_n2[1] = *ilit; ++ilit;
  zimn_otstupka_n3[0] = *ilit; ++ilit;
  zimn_otstupka_n3[1] = *ilit; ++ilit;
  zimn_otstupka_n3[2] = *ilit; ++ilit;
  zimn_otstupka_n4[0] = *ilit; ++ilit;
  zimn_otstupka_n4[1] = *ilit; ++ilit;
  zimn_otstupka_n4[2] = *ilit; ++ilit;
  zimn_otstupka_n4[3] = *ilit; ++ilit;
  zimn_otstupka_n5[0] = *ilit; ++ilit;
  zimn_otstupka_n5[1] = *ilit; ++ilit;
  zimn_otstupka_n5[2] = *ilit; ++ilit;
  zimn_otstupka_n5[3] = *ilit; ++ilit;
  zimn_otstupka_n5[4] = *ilit; ++ilit;
  osen_otstupka   [0] = *ilit; ++ilit;
  osen_otstupka   [1] = *ilit; ++ilit;
  //ctor internal data structures
  struct DayData {
    int8_t dn;
    int8_t glas;
    int8_t n50;
    ApEvReads apostol;
    ApEvReads evangelie;
    std::set<uint16_t> day_markers;
    DayData() : dn{-1}, glas{-1}, n50{-1} {}
    DayData(int8_t x) : dn{x}, glas{-1}, n50{-1} {}
  };
  std::map<ShortDate, DayData> days;
  std::multimap<uint16_t, ShortDate> markers;
  const auto pasha_date = pasha_calc(y);
  const auto pasha_date_pred = pasha_calc(y-1);
  auto is_visokos = [](const big_int& y) { return (y%4)==0; };
  const bool b = is_visokos(y);
  const bool b1 = is_visokos(y-1);
  ShortDate nachalo_posta, t1, t2, t3;
  ShortDate dd {pasha_date};
  int i = 0, j = 0, glas = 8;
  bool f = false;
  //карта дней недели пред. года
  std::map<ShortDate, int8_t> dn_prev;
  //функц.возвращает кол-во дней в месяце или -1
  auto get_days_inmonth_ = [](int8_t month, bool leap) -> int8_t {
    int8_t k{-1};
    switch(month) {
      case 1: { k = 31; }
          break;
      case 2: { k = (leap ? 29 : 28); }
          break;
      case 3: { k = 31; }
          break;
      case 4: { k = 30; }
          break;
      case 5: { k = 31; }
          break;
      case 6: { k = 30; }
          break;
      case 7: { k = 31; }
          break;
      case 8: { k = 31; }
          break;
      case 9: { k = 30; }
          break;
      case 10:{ k = 31; }
          break;
      case 11:{ k = 30; }
          break;
      case 12:{ k = 31; }
    };
    return k;
  };
  //функц.возвращает дату увеличенную на кол-во дней доконца года
  //если инкремент переходит через конец года, возвращает исходную дату
  //date.first - месяц; date.second - день
  auto increment_date_ = [&get_days_inmonth_](ShortDate date, int days, bool visokos) -> ShortDate {
    ShortDate r {date};
    ShortDate l {date};
    if(days<1) { return l; }
    if(date.first<1 || date.first>12)  { return l; }
    int u = get_days_inmonth_(date.first, visokos);
    if(date.second<1 || date.second>u) { return l; }
    r.second += days;
    while (r.second>u) {
      r.second = r.second - u;
      r.first++;
      if(r.first>12) return l;
      u = get_days_inmonth_(r.first, visokos);
    }
    return r;
  };
  //функц.возвращает дату уменьшенную на кол-во дней до начала года
  //если декремент переходит через начало года, возвращает исходную дату
  //date.first - месяц; date.second - день
  auto decrement_date_ = [&get_days_inmonth_](ShortDate date, int days, bool visokos) -> ShortDate {
    ShortDate r {date};
    ShortDate l {date};
    if(days<1) { return l; }
    if(date.first<1 || date.first>12)  { return l; }
    int u = get_days_inmonth_(date.first, visokos);
    if(date.second<1 || date.second>u) { return l; }
    r.second = r.second - days;
    while (r.second < 1) {
      r.first--;
      if(r.first<1) return l;
      u = get_days_inmonth_(r.first, visokos);
      r.second = u + r.second;
    }
    return r;
  };
  //функц.создание карты дней недели указанного года в формате:
  //key - дата; key.first - месяц; key.second - день
  //value - деньнедели; 0-вс, 1-пн, 2-вт, 3-ср, 4-чт, 5-пт, 6-сб.
  auto create_days_map_ = [&increment_date_, &decrement_date_, &is_visokos, this]
                          (const big_int& y) -> std::optional<std::map<ShortDate, int8_t>>
  {
    if(y<1) return std::nullopt;
    const bool b = is_visokos(y);
    const auto pasha_date = pasha_calc(y) ;
    ShortDate data1, data2;
    std::map<ShortDate, int8_t> result;
    int i = 0;
    while (i<7) {
      data1 = increment_date_(pasha_date, i, b);
      data2 = increment_date_(data1, 7, b);
      result.insert({data1, i});
      while (data1 != data2) {
        result.insert({data2, i});
        data1 = data2;
        data2 = increment_date_(data1, 7, b);
      }
      data1 = decrement_date_(pasha_date, 7-i, b);
      if(data1 != pasha_date) result.insert({data1, i});
      data2 = decrement_date_(data1, 7, b);
      while (data2 != data1) {
        result.insert({data2, i});
        data1 = data2;
        data2 = decrement_date_(data1, 7, b);
      }
      i++;
    }
    return result;
  };
  //функц.установка признака для даты
  auto add_marker_for_date_ = [&days, &markers](const ShortDate& d, oxc_const m){
    #ifdef NDEBUG
    if(auto fr = days.find(d); fr!=days.end()) {
      fr->second.day_markers.insert(m);
      markers.insert({m, d});
    }
    #else
    if(auto fr = days.find(d); fr!=days.end()) {
      auto [it, ok] = fr->second.day_markers.insert(m);
      assert((void("days container insertion failed"), ok));
      assert((void("markers container insertion failed"),
              std::none_of(markers.begin(), markers.end(), [d,m](const auto& e){ return m==e.first && d==e.second; })));
      markers.insert({m, d});
      assert(fr->second.day_markers.size() <= M_COUNT);
    } else {
      assert((void("element not found"), false));
    }
    #endif
  };
  //функц.установка нескольких признакoB для даты
  auto add_markers_for_date_ = [&days, &markers](const ShortDate& d, std::initializer_list<uint16_t> l){
    #ifdef NDEBUG
    if(auto fr = days.find(d); fr!=days.end()) {
      for(auto i: l) {
        fr->second.day_markers.insert(i);
        markers.insert({i, d});
      }
    }
    #else
    if(auto fr = days.find(d); fr!=days.end()) {
      for(auto i: l) {
        auto [it, ok] = fr->second.day_markers.insert(i);
        assert((void("days container insertion failed"), ok));
        assert((void("markers container insertion failed"),
                std::none_of(markers.begin(), markers.end(), [d,i](const auto& e){ return i==e.first && d==e.second; })));
        markers.insert({i, d});
        assert(fr->second.day_markers.size() <= M_COUNT);
      }
    } else {
      assert((void("element not found"), false));
    }
    #endif
  };
  //функц.поиск дня недели
  auto get_dn_ = [&days](const ShortDate& d)->int8_t{
    if(auto e = days.find(d); e!=days.end()) return e->second.dn;
    else return -1;
  };
  auto get_dn_prev_year = [&dn_prev](const ShortDate& d)->int8_t{
    if(auto e = dn_prev.find(d); e!=dn_prev.end()) return e->second;
    else return -1;
  };
  //функц.проверки даты на признак
  auto check_date_ = [&days](const ShortDate& d, oxc_const m){
    if(auto fr = days.find(d); fr!=days.end()) {
      return fr->second.day_markers.contains(m);
    } else {
      return false;
    }
  };
  //функц.поиск даты попризнаку
  auto get_date_ = [&markers](oxc_const m)->ShortDate {
    if(auto fr = markers.find(m); fr!=markers.end()) {
      return fr->second;
    } else {
      return ShortDate(-1, -1);
    }
  };
  //функц.установка гласа для даты
  auto set_glas_ = [&days](const ShortDate& d, const int8_t glas){
    if(auto fr = days.find(d); fr!=days.end()) {
      fr->second.glas = glas;
    } else { assert((void("element not found"), false)); }
  };
  //функц.установка евангелия для даты
  auto set_evangelie_ = [&days](const ShortDate& d, const ApEvReads& ev){
    if(auto fr = days.find(d); fr!=days.end()) {
      fr->second.evangelie = ev;
    } else { assert((void("element not found"), false)); }
  };
  //функц.установка апостола для даты
  auto set_apostol_ = [&days](const ShortDate& d, const ApEvReads& ap){
    if(auto fr = days.find(d); fr!=days.end()) {
      fr->second.apostol = ap;
    } else { assert((void("element not found"), false)); }
  };
  //функц.установка номер по пятидесятнице для даты
  auto set_n50_ = [&days](const ShortDate& d, const int8_t n){
    if(auto fr = days.find(d); fr!=days.end()) {
      fr->second.n50 = n;
    } else { assert((void("element not found"), false)); }
  };
  //функц.поиск номер по пятидесятнице для даты
  auto get_n50_ = [&days](const ShortDate& d)->int8_t{
    if(auto e = days.find(d); e!=days.end()) return e->second.n50;
    else return -1;
  };
  //создание карт дней недели всего года
  if(auto x = create_days_map_(y)) {
    std::transform(x->cbegin(), x->cend(), std::inserter(days, days.end()), [](const auto& e){
      return std::make_pair(e.first, DayData{e.second});
    });
  }
  if(auto x = create_days_map_(y-1)) {
    dn_prev = std::move(*x);
  }
  //расчет дат непереходящих праздников
  for(auto it = stable_dates.begin(); it != stable_dates.end(); it = std::next(it,3)) {
    add_marker_for_date_(ShortDate(*std::next(it), *std::next(it,2)), *it);
  }
  for(const auto& x: svyatki_dates) add_marker_for_date_(x, full7_svyatki);
  for( auto x=make_pair(11,15), y=make_pair(12,25); x<y; x=increment_date_(x,1,b) ) {
    add_marker_for_date_(x, post_rojd);
  }
  for( auto x=make_pair(8,1), y=make_pair(8,15); x<y; x=increment_date_(x,1,b) ) {
    add_marker_for_date_(x, post_usp);
  }
  //расчет дат от пасхи до дня всех всятых
  add_markers_for_date_(dd, {pasha, full7_pasha});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {svetlaya1, full7_pasha});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {svetlaya2, full7_pasha, mari_icon_09, mari_icon_17, prep_dav_gar, hristodul});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {svetlaya3, full7_pasha, mari_icon_24, sobor_sinai_prep});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {svetlaya4, full7_pasha});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {svetlaya5, full7_pasha, mari_icon_06});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {svetlaya6, full7_pasha});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, ned2_popashe);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s2popashe_1);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s2popashe_2);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s2popashe_3);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s2popashe_4);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s2popashe_5);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s2popashe_6);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {ned3_popashe, iosif_arimaf, tamar_gruz});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s3popashe_1);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s3popashe_2);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s3popashe_3);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s3popashe_4);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s3popashe_5);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s3popashe_6);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {ned4_popashe, tavif, pm_avraam_bolg});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s4popashe_1);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s4popashe_2);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s4popashe_3, mari_icon_04, mari_icon_14});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s4popashe_4);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s4popashe_5);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s4popashe_6, sobor_butov});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, ned5_popashe);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s5popashe_1);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s5popashe_2);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s5popashe_3);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s5popashe_4);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s5popashe_5);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s5popashe_6);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, ned6_popashe);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s6popashe_1);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s6popashe_2, mari_icon_07});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s6popashe_3);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s6popashe_4, much_fereidan});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s6popashe_5);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s6popashe_6);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {ned7_popashe, mari_icon_23, mari_icon_25});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s7popashe_1);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s7popashe_2);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s7popashe_3, dodo_gar});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s7popashe_4, david_gar});
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s7popashe_5);
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, s7popashe_6);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {ned8_popashe, full7_troica});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s1po50_1, full7_troica, mari_icon_12, mari_icon_20});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s1po50_2, full7_troica});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s1po50_3, full7_troica});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s1po50_4, full7_troica, mari_icon_19});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s1po50_5, full7_troica});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {s1po50_6, full7_troica});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {ned1_po50, mari_icon_22, mari_icon_10, mari_icon_05, mari_icon_16});
  for( auto x=increment_date_(dd,1,b), y=make_pair(6,29); x<y; x=increment_date_(x,1,b) ) {
    add_marker_for_date_(x, post_petr);
  }
  //четверг 2-й седмицы по Пятидесятнице
  dd = increment_date_(dd, 4, b);
  add_marker_for_date_(dd, mari_icon_15);
  //1-я пятница Петрова поста - Прп. Варлаама Хутынского
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {varlaam_hut, mari_icon_08, mari_icon_21});
  //всех святых, в земле Русской просиявших
  dd = increment_date_(dd, 2, b);
  add_markers_for_date_(dd, {ned2_po50, sobor_vsehsv_rus, sobor_afonpr});
  //Собор Белорусских, Вологодских, Новгородских, Псковских , Петербургских, Удмуртских, Волгоградских святых
  dd = increment_date_(dd, 7, b);
  add_markers_for_date_(dd, {ned3_po50, sobor_belorus, sobor_vologod,
                              sobor_novgorod, sobor_pskov, sobor_piter, sobor_udmurt, sobor_volgograd});
  //Неделя 4-я по Пятидесятнице. дня преподобных отцов Псково-Печерских
  dd = increment_date_(dd, 7, b);
  add_markers_for_date_(dd, {ned4_po50, sobor_ppech_prep});
  //Соборa Валаамских святых
  dd = make_pair(8,7);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_valaam);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //празники на 29 или 28 февраля
  dd = b ? make_pair(2,29) : make_pair(2,28) ;
  add_marker_for_date_(dd, mari_icon_03);
  //иконы Божией Матери «Казанская Коробейниковская»
  dd = make_pair(6,18);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, mari_icon_13);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //иконы Божией Матери Прибавление Ума
  dd = make_pair(8,16);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, mari_icon_18);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор святых Челябинской митрополии (воскресенье после 27 сентября);
  //    Празднование совершается: 27 сентября, если это воскресный день; в воскресенье перед 27 сентября,
  //      если на воскресенье после 27 сентября приходится праздник Покрова Божией Матери.
  dd = make_pair(9,27);
  do {
    i = get_dn_(dd);
    if(i==0) {
      if(dd!=make_pair(10,1)) {
        add_marker_for_date_(dd, sobor_chelyab);
      } else {
        dd = make_pair(9,26);
        do {
          i = get_dn_(dd);
          if(i==0) {
            add_marker_for_date_(dd, sobor_chelyab);
            break;
          }
          dd = decrement_date_(dd, 1, b);
        } while (true);
      }
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор новомучеников и исповедников Казахстанских
  dd = make_pair(9,3);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_kazahst);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор новомучеников и исповедников земли Карельской
  dd = make_pair(10,18);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_karel);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор святых Пермской митрополии
  dd = make_pair(1,29);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_perm);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор Московских святых
  dd = make_pair(8,25);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_mosk);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //Собор Смоленских святых
  dd = make_pair(7,27);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_smolensk);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //Собор святых Нижегородской митрополии
  dd = make_pair(8,26);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_nnovgor);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор мучеников Холмских и Подляшских
  dd = make_pair(5,19);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_much_holm);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор Саратовских святых (ближайшее воскресенье к Ав. 31)
  dd = make_pair(8,31);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_saratov);
    }
    break;
    case 1: {}
    case 2: {}
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_saratov);
          break;
        }
      } while(true);
    }
    break;
    case 4: {}
    case 5: {}
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_saratov);
          break;
        }
      } while(true);
    }
    break;
    default: {}
  };
  //Собор Аланских святых (ближайшее воскресенье к Н. 10)
  dd = make_pair(11,10);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_alansk);
    }
    break;
    case 1: {}
    case 2: {}
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_alansk);
          break;
        }
      } while(true);
    }
    break;
    case 4: {}
    case 5: {}
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_alansk);
          break;
        }
      } while(true);
    }
    break;
    default: {}
  };
  //Собор святых, в земле Германской просиявших (ближайшее воскресенье к C. 20)
  dd = make_pair(9,20);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_german);
    }
    break;
    case 1: {}
    case 2: {}
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_german);
          break;
        }
      } while(true);
    }
    break;
    case 4: {}
    case 5: {}
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_german);
          break;
        }
      } while(true);
    }
    break;
    default: {}
  };
  //Перенесение мощей блгвв. кн. Петра и Февронии
  dd = make_pair(9,6);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, petr_fevron_murom);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //Собор святых Кубанской митрополии; Собор святых, в земле Испанской и Португальской просиявших
  dd = make_pair(9,28);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_markers_for_date_(dd, {sobor_kuban, sobor_ispan});
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //Суббота перед Воздви́жение
  dd = make_pair(9,13);
  do {
    i = get_dn_(dd);
    if(i==6) {
      add_marker_for_date_(dd, sub_pered14sent);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //неделя перед Воздви́жение
  dd = make_pair(9,13);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, ned_pered14sent);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //Суббота после Воздви́жение
  dd = make_pair(9,15);
  do {
    i = get_dn_(dd);
    if(i==6) {
      add_marker_for_date_(dd, sub_po14sent);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //неделя после Воздви́жение
  dd = make_pair(9,15);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, ned_po14sent);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Святых отец 7 вселенск соборa
  dd = make_pair(10,11);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_otcev7sobora);
    }
    break;
    case 1: {}
    case 2: {}
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_otcev7sobora);
          break;
        }
      } while(true);
    }
    break;
    case 4: {}
    case 5: {}
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_otcev7sobora);
          break;
        }
      } while(true);
    }
    break;
    default: {}
  };
  //дмитриевская родительская суббота
  dd = make_pair(10,25);
  do {
    i = get_dn_(dd);
    if(i==6 && dd.second!=22) {
      add_marker_for_date_(dd, sub_dmitry);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //собор безсребреников
  dd = make_pair(11,1);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_bessrebren);
    }
    break;
    case 1: { }
    case 2: { }
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_bessrebren);
          break;
        }
      } while(true);
    }
    break;
    case 4: { }
    case 5: { }
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_bessrebren);
          break;
        }
      } while(true);
    }
    break;
    default: { }
  };
  //нед.св.отец перед рождеством от 18до24 дек.
  dd = make_pair(12,24);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, ned_peredrojd);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //неделя св.праотец от11до17 дек.
  dd = decrement_date_(dd, 1, b);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, ned_praotec);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //Суббота перед рождеством
  dd = make_pair(12,24);
  do {
    i = get_dn_(dd);
    if(i==6) {
      add_marker_for_date_(dd, sub_peredrojd);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //неделя мытаря и фарисея
  dd = decrement_date_(pasha_date, 70, b);
  add_markers_for_date_(dd, {ned_mitar_ifaris, full7_mitar});
  for(auto x = increment_date_(dd, 1, b), y = increment_date_(dd, 7, b); x<y; x = increment_date_(x, 1, b)) {
    add_marker_for_date_(x, full7_mitar);
  }
  //неделя о блудномсыне
  dd = increment_date_(dd, 7, b);
  add_marker_for_date_(dd, ned_obludnom);
  //вселенская родительская суббота, мясопустная
  dd = increment_date_(dd, 6, b);
  add_marker_for_date_(dd, sub_myasopust);
  t1 = dd;
  //от недели мясопустной до вел.субботы
  dd = increment_date_(dd, 1, b);
  add_marker_for_date_(dd, ned_myasopust);
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {sirnaya1, full7_sirn});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {sirnaya2, full7_sirn});
  dd = increment_date_(dd, 1, b);
  t2 = dd;
  add_markers_for_date_(dd, {sirnaya3, full7_sirn});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {sirnaya4, full7_sirn, shio_mg});
  dd = increment_date_(dd, 1, b);
  t3 = dd;
  add_markers_for_date_(dd, {sirnaya5, full7_sirn});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {sirnaya6, full7_sirn, sobor_vseh_prep});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {ned_siropust, full7_sirn});
  dd = increment_date_(dd, 1, b);
  nachalo_posta = dd;
  add_markers_for_date_(dd, {vel_post_d1n1, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n1, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n1, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n1, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n1, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n1, post_vel, feodor_tir});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d0n2, post_vel, mari_icon_11});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d1n2, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n2, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n2, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n2, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n2, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n2, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d0n3, post_vel, grigor_palam, sobor_kpech_prep});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d1n3, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n3, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n3, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n3, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n3, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n3, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d0n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d1n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n4, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d0n5, post_vel, ioann_lestv});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d1n5, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n5, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n5, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n5, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n5, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n5, post_vel, mari_icon_01, mari_icon_02});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d0n6, post_vel, mari_egipt});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d1n6, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n6, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n6, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n6, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n6, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n6, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d0n7, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d1n7, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d2n7, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d3n7, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d4n7, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d5n7, post_vel});
  dd = increment_date_(dd, 1, b);
  add_markers_for_date_(dd, {vel_post_d6n7, post_vel});
  // Суббота по Рождестве (типикон стр.380)
  i = get_dn_(make_pair(12,25));
  switch(i) {
    case 1: { dd = make_pair(12,30); } break;
    case 2: { dd = make_pair(12,29); } break;
    case 3: { dd = make_pair(12,28); } break;
    case 4: { dd = make_pair(12,27); } break;
    case 5: { dd = make_pair(12,26); } break;
    default: { dd = make_pair(12,31); }
  };
  switch(get_dn_(dd)) {
    case 6: { add_marker_for_date_(dd, sub_porojdestve); } break;
    default: { add_marker_for_date_(dd, sub_porojdestve_r); }
  };
  // неделя по Рождестве (типикон стр.380)
  switch(i) {
    case 1: { dd = make_pair(12,31); } break;
    case 2: { dd = make_pair(12,30); } break;
    case 3: { dd = make_pair(12,29); } break;
    case 4: { dd = make_pair(12,28); } break;
    case 5: { dd = make_pair(12,27); } break;
    default: { dd = make_pair(12,26); }
  };
  switch(get_dn_(dd)) {
    case 0:  { add_markers_for_date_(dd, {ned_porojdestve, ned_prav_bogootec}); } break;
    default: { add_markers_for_date_(dd, {ned_porojdestve_r, ned_prav_bogootec}); }
  };
  // Суббота пред Богоявлением (типикон стр.380)
  if(i==0 || i==1) {
    dd = i==1 ? make_pair(12,30) : make_pair(12,31) ;
    switch(get_dn_(dd)) {
      case 6: { add_marker_for_date_(dd, sub_peredbogoyav); } break;
      default: { add_marker_for_date_(dd, sub_peredbogoyav_r); }
    };
  }
  i = get_dn_prev_year(make_pair(12,25)) ;
  if(!(i==0 || i==1)) {
    switch(i) {
      case 2: { dd = make_pair(1,5); } break;
      case 3: { dd = make_pair(1,4); } break;
      case 4: { dd = make_pair(1,3); } break;
      case 5: { dd = make_pair(1,2); } break;
      default: { dd = make_pair(1,1); }
    };
    switch(get_dn_(dd)) {
      case 6: { add_marker_for_date_(dd, sub_peredbogoyav); } break;
      default: { add_marker_for_date_(dd, sub_peredbogoyav_r); }
    };
  }
  // неделя пред Богоявлением (типикон стр.380)
  switch(i) {
    case 3: { dd = make_pair(1,5); } break;
    case 4: { dd = make_pair(1,4); } break;
    case 5: { dd = make_pair(1,3); } break;
    case 6: { dd = make_pair(1,2); } break;
    default: { dd = make_pair(1,1); }
  };
  switch(get_dn_(dd)) {
    case 0: { add_marker_for_date_(dd, ned_peredbogoyav); } break;
    default: { add_marker_for_date_(dd, ned_peredbogoyav_r); }
  };
  //Суббота пo Богоявление
  dd = make_pair(1,7);
  do {
    i = get_dn_(dd);
    if(i==6) {
      add_markers_for_date_(dd, {sub_pobogoyav, pahomii_kensk});
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //неделя пo Богоявление
  dd = make_pair(1,7);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, ned_pobogoyav);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //собор новомучеников русской церкви
  dd = make_pair(1,25);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_novom_rus);
    }
    break;
    case 1: { }
    case 2: { }
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_novom_rus);
          break;
        }
      } while(true);
    }
    break;
    case 4: { }
    case 5: { }
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_novom_rus);
          break;
        }
      } while(true);
    }
    break;
    default: {}
  };
  //собор 3-x святителей
  dd = make_pair(1, 30);
  if(dd==t1 || dd==t2 || dd==t3) dd = make_pair(1, 29);
  add_marker_for_date_(dd, sobor_3sv);
  //Сре́тение Господа Бога и Спаса нашего Иисуса Христа
  dd = make_pair(2, 2);
  if(dd>=nachalo_posta) dd = decrement_date_(nachalo_posta, 1, b);
  add_marker_for_date_(dd, sretenie);
  if(dd==t1) {
    // если сретение и вселенская родительская суббота выпали на один день
    // то перемещаем субботу на неделю раньше
    if(auto fr = days.find(t1); fr!=days.end()) {
      fr->second.day_markers.erase(sub_myasopust);
      markers.erase(sub_myasopust);
      t1 = decrement_date_(t1, 1, b);
      do {
        i = get_dn_(t1);
        if(i==6) {
          add_marker_for_date_(t1, sub_myasopust);
          break;
        }
        t1 = decrement_date_(t1, 1, b);
      } while (true);
    }
  }
  //Предпразднство Сре́тения
  if(dd != make_pair(2, 1)) {
    dd = make_pair(2, 1);
    if(dd==t1) dd = decrement_date_(dd, 1, b);
    add_marker_for_date_(dd, sretenie_predpr);
  }
  //отдание праздника Сре́тения
  dd = get_date_(sretenie);
  t3 = make_pair(2, 9);
  t1 = get_date_(ned_obludnom);
  t2 = increment_date_(t1, 2, b);
  if(dd>=t1 && dd<=t2) {
    t3 = increment_date_(t1, 5, b);
  }
  t1 = increment_date_(t1, 3, b);
  t2 = increment_date_(t1, 3, b);
  if(dd>=t1 && dd<=t2) {
    t3 = get_date_(sirnaya2);
  }
  t1 = get_date_(ned_myasopust);
  t2 = get_date_(sirnaya1);
  if(dd>=t1 && dd<=t2) {
    t3 = get_date_(sirnaya4);
  }
  t1 = get_date_(sirnaya2);
  t2 = get_date_(sirnaya3);
  if(dd>=t1 && dd<=t2) {
    t3 = get_date_(sirnaya6);
  }
  t1 = get_date_(sirnaya4);
  t2 = get_date_(sirnaya6);
  if(dd>=t1 && dd<=t2) {
    t3 = get_date_(ned_siropust);
  }
  if(!check_date_(dd, ned_siropust)) {
    if(check_date_(t3, sub_myasopust)) t3 = decrement_date_(t3, 1, b);
    add_marker_for_date_(t3, sretenie_otdanie);
  }
  //Попразднствa Сретения Господня
  t3 = get_date_(sretenie_otdanie);
  t1 = increment_date_(dd, 1, b);
  t2 = t1;
  i = 1;
  if(t3!=make_pair(-1,-1) && t3!=t1) {
    do {
      if(check_date_(t2, sub_myasopust)) {
        t2 = increment_date_(t2, 1, b);
        if(t2>=t3) break;
      }
      switch(i) {
        case 1: { add_marker_for_date_(t2, sretenie_poprazd1); }
        break;
        case 2: { add_marker_for_date_(t2, sretenie_poprazd2); }
        break;
        case 3: { add_marker_for_date_(t2, sretenie_poprazd3); }
        break;
        case 4: { add_marker_for_date_(t2, sretenie_poprazd4); }
        break;
        case 5: { add_marker_for_date_(t2, sretenie_poprazd5); }
        break;
        case 6: { add_marker_for_date_(t2, sretenie_poprazd6); }
        break;
        default:{ }
      };
      t2 = increment_date_(t2, 1, b);
      i++;
    } while (t2!=t3);
  }
  //Первое и второе Обре́тение главы Иоанна Предтечи
  dd = make_pair(2, 24);
  if( check_date_(dd, sub_myasopust) || check_date_(dd, sirnaya3) ||
      check_date_(dd, sirnaya5) || check_date_(dd, vel_post_d1n1) )
  {
    dd = make_pair(2, 23);
  }
  t1 = get_date_(vel_post_d2n1);
  t2 = get_date_(vel_post_d5n1);
  if(dd>=t1 && dd<=t2) dd = get_date_(vel_post_d6n1);
  add_marker_for_date_(dd, obret_gl_ioanna12);
  //Святых сорока́ мучеников, в Севасти́йском е́зере мучившихся
  dd = make_pair(3, 9);
  if(check_date_(dd, vel_post_d3n4)) dd = make_pair(3, 8);
  if(check_date_(dd, vel_post_d4n5)) dd = make_pair(3, 7);
  if(check_date_(dd, vel_post_d6n5)) dd = make_pair(3, 10);
  t1 = get_date_(vel_post_d1n1);
  t2 = get_date_(vel_post_d5n1);
  if(dd>=t1 && dd<=t2) dd = get_date_(vel_post_d6n1);
  add_marker_for_date_(dd, muchenik_40);
  //предпразднество Благовещ́ение Пресвято́й Богоро́дицы
  dd = make_pair(3, 24);
  t1 = get_date_(vel_post_d1n7);
  t2 = make_pair(3, 25);
  if(t2<t1) {
    if(check_date_(dd, vel_post_d6n6)) dd = make_pair(3, 22);
    if(check_date_(dd, vel_post_d4n5)) dd = make_pair(3, 23);
    if(check_date_(dd, vel_post_d2n5)) dd = make_pair(3, 23);
    add_marker_for_date_(dd, blag_predprazd);
  }
  //отдание праздника Благовещ́ение
  dd = make_pair(3, 26);
  t1 = get_date_(vel_post_d6n6);
  if(dd<t1) {
    add_marker_for_date_(dd, blag_otdanie);
  }
  //Вмч. Гео́ргия Победоно́сца. Мц. царицы Александры
  dd = make_pair(4, 23);
  t1 = get_date_(vel_post_d1n7);
  t2 = get_date_(pasha);
  if(dd>=t1 && dd<=t2) {
    dd = get_date_(svetlaya1);
  }
  add_marker_for_date_(dd, georgia_pob);
  //третье Обре́тение главы Иоанна Предтечи.
  dd = make_pair(5, 25);
  t1 = get_date_(s7popashe_6);
  t2 = get_date_(ned1_po50);
  if(dd==t1 || dd==t2) dd = make_pair(5, 23);
  if(check_date_(dd, s1po50_1)) dd = make_pair(5, 26);
  if(check_date_(dd, ned8_popashe)) dd = make_pair(5, 22);
  add_marker_for_date_(dd, obret_gl_ioanna3);
  //Прмчч Липсийских(переходящее празднование в 1-е воскресенье после 27 июня).
  dd = make_pair(6, 27);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, much_lipsiisk);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор Алтайских святых.
  dd = make_pair(9, 7);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_altai);
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Собор Тверских святых;
  //Свт.Арсения, еп. Тверского
  //Прпп. Тихона, Василия и Никона Соколовских(XVI) (переходящее празднование в 1-е воскресенье после 29 июня).
  dd = make_pair(6, 30);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_markers_for_date_(dd, {sobor_tversk, prep_sokolovsk, arsen_tversk});
      break;
    }
    dd = increment_date_(dd, 1, b);
  } while (true);
  //Святых отец 6-и вселенских соборов
  dd = make_pair(7, 16);
  i = get_dn_(dd);
  switch (i) {
    case 0: {
      add_marker_for_date_(dd, sobor_otcev_1_6sob);
    }
    break;
    case 1: { }
    case 2: { }
    case 3: {
      do {
        dd = decrement_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_otcev_1_6sob);
          break;
        }
      } while(true);
    }
    break;
    case 4: { }
    case 5: { }
    case 6: {
      do {
        dd = increment_date_(dd, 1, b);
        i = get_dn_(dd);
        if(i==0) {
          add_marker_for_date_(dd, sobor_otcev_1_6sob);
          break;
        }
      } while(true);
    }
    break;
    default: { }
  };
  //Собор Кузбасских святых (последний воскресный день августа)
  dd = make_pair(8, 31);
  do {
    i = get_dn_(dd);
    if(i==0) {
      add_marker_for_date_(dd, sobor_kuzbas);
      break;
    }
    dd = decrement_date_(dd, 1, b);
  } while (true);
  //расчет Двунадесятые переходящие праздники
  add_marker_for_date_(get_date_(vel_post_d0n7),     dvana10_per_prazd);
  add_marker_for_date_(get_date_(s6popashe_4),       dvana10_per_prazd);
  add_marker_for_date_(get_date_(ned8_popashe),      dvana10_per_prazd);
  //расчет Двунадесятые непереходящие праздники
  add_marker_for_date_(get_date_(m1d6),     dvana10_nep_prazd);
  add_marker_for_date_(get_date_(sretenie), dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m3d25),    dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m8d6),     dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m8d15),    dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m9d8),     dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m9d14),    dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m11d21),   dvana10_nep_prazd);
  add_marker_for_date_(get_date_(m12d25),   dvana10_nep_prazd);
  //расчет великие праздники
  add_marker_for_date_(get_date_(m1d1),    vel_prazd);
  add_marker_for_date_(get_date_(m6d24),   vel_prazd);
  add_marker_for_date_(get_date_(m6d29),   vel_prazd);
  add_marker_for_date_(get_date_(m8d29),   vel_prazd);
  add_marker_for_date_(get_date_(m10d1),   vel_prazd);
  //расчет гласов период от субб. лазарева до нед. всех святых
  t1 = get_date_(vel_post_d6n6);//субб лазарева
  t2 = get_date_(ned1_po50);//нед всех святых
  dd = t1;
  do {
    set_glas_(dd, -1); //неопределенный глас: -1
    dd = increment_date_(dd, 1, b);
  } while(dd<=t2);
  //расчет гласов период от начала петрова поста до конца года
  dd = increment_date_(t2, 1, b);
  j = get_dn_(dd);
  do {
    do {
      set_glas_(dd, glas);
      t3 = dd;
      dd = increment_date_(dd, 1, b);
      if(dd==t3) { f = true; break; }
      j = get_dn_(dd);
    } while(j>0);
    if(f || j<0) break;
    glas++;
    if(glas>8) glas = 1;
  } while(true);
  //расчет гласов период от начала года до субб. лазарева
  t1 = pasha_date_pred;
  dd = increment_date_(t1, 57, b1);
  f = false;
  j = 1;
  glas = 8;
  do {
    do {
      t3 = dd;
      dd = increment_date_(dd, 1, b1);
      if(dd==t3) { f = true; break; }
      j = get_dn_prev_year(dd);
    } while(j>0);
    if(f || j<0) break;
    glas++;
    if(glas>8) glas = 1;
  } while(true);
  dd = make_pair(1, 1);
  j = get_dn_(dd);
  t1 = get_date_(vel_post_d6n6);
  if(j<1) {
    glas++;
    if(glas>8) glas = 1;
  }
  f = false;
  do {
    do {
      set_glas_(dd, glas);
      dd = increment_date_(dd, 1, b);
      if(dd==t1) { f = true; break; }
      j = get_dn_(dd);
    } while(j>0);
    if(f || j<0) break;
    glas++;
    if(glas>8) glas = 1;
  } while(true);
  //расчет календарный номер по пятидесятнице каждого дня года
  t1 = increment_date_(pasha_date_pred, 49, b1);
  i = 0;
  while(true) {
    t2 = increment_date_(t1, 1, b1);
    if(t2!=t1) { t1 = t2; }
    else       { break; }
    if(get_dn_prev_year(t1)==1) i++; //i = номер для 31дек. пред. года
  }
  t1 = make_pair(1, 1);
  if(get_dn_(t1)==1) i++;//i = номер для 1янв. текущ. года
  nachalo_posta = get_date_(vel_post_d1n1);//началo вел.поста
  dd = get_date_(ned8_popashe);//пятидесятницa
  while(true) {
    if(t1<nachalo_posta) {
      set_n50_(t1, i);
    } else if(t1>=nachalo_posta && t1<dd) {
      //для периода от начала вел.поста до тр.род.субб. вкл.
      set_n50_(t1, -1);
    } else if(t1==dd) {
      //для пятидесятницы
      set_n50_(t1, 0);
      i = 0;
    } else {
      //для периода от пятидесятницы до конца года
      set_n50_(t1, i);
    }
    t2 = increment_date_(t1, 1, b);
    if(t2!=t1) { t1 = t2; }
    else       { break; }
    if(get_dn_(t1)==1) i++;
  }
  //расчет рядовые чтения евангелия на литургии
  i = j = 0;
  std::vector<int> v, w, v1, w1;//контейнеры для номеров доб.седмиц и недель
  t1 = make_pair(1, 1);
  t3 = make_pair(9, 15);
  dd = get_date_(ned_mitar_ifaris);
  auto ddd = increment_date_(pasha_date_pred, 49, b1);//пятидесятницa пред. года
  auto d2 {get_date_(ned_pobogoyav)};
  auto mf7  {increment_date_(dd,7,b)};
  auto mf14 {increment_date_(dd,14,b)};
  auto mf21 {increment_date_(dd,21,b)};
  auto ned_po_vozdv {get_date_(ned_po14sent)};
  auto dd1 {decrement_date_(ned_po_vozdv, 14, b)};
  auto dd2 {decrement_date_(ned_po_vozdv,  7, b)};
  const int kdn {get_dn_({1, 6})};
  while (true) {//поиск t3 = датa недели по воздвижении пред. года
    int q{ get_dn_prev_year(t3) };
    if(q==0) { break; }
    t3 = increment_date_(t3, 1, b1);
  }
  do {//поиск i = календарный номер t3 по пятидесятнице пред. года
    ddd = increment_date_(ddd, 7, b1);
    i++;
  } while(ddd!=t3);
  int sn   {17-i};//кол-во седмиц осенней отступки/преступки  пред. года
  int osen {17 - get_n50_(ned_po_vozdv)};//тоже для текущего года
  int zimn {};//расчет кол-во седмиц зимней отступки (А.Кашкин - стр.126)
  if( !(dd==d2 && kdn!=0 && kdn!=1) ) {
    if( dd==d2 && (kdn==0||kdn==1) ) zimn--;
    if( dd!=d2 ) {
      if(kdn==0 || kdn==1) zimn--;
      auto d3 {d2};
      while(d3!=dd) {
        d3 = increment_date_(d3, 7, b);
        zimn--;
      }
    }
  }
  if(zimn!=0) {//поиск ddd = дата начала нового ряда чтений
    switch(kdn) {
    case 1: {  }
    case 0: { ddd = {1, 7}; } break;
    default:{ ddd = increment_date_(d2, 1, b); } };
  } else {
    ddd = {-1,-1};
  }
  switch(zimn) {//выбор номеров добавочных седмиц из опций класса
    case -1: { v.push_back(zimn_otstupka_n1); } break;
    case -2: {
      v.resize(zimn_otstupka_n2.size());
      std::reverse_copy(zimn_otstupka_n2.begin(), zimn_otstupka_n2.end(), std::begin(v));
    } break;
    case -3: {
      v.resize(zimn_otstupka_n3.size());
      std::reverse_copy(zimn_otstupka_n3.begin(), zimn_otstupka_n3.end(), std::begin(v));
    } break;
    case -4: {
      v.resize(zimn_otstupka_n4.size());
      std::reverse_copy(zimn_otstupka_n4.begin(), zimn_otstupka_n4.end(), std::begin(v));
    } break;
    case -5: {
      v.resize(zimn_otstupka_n5.size());
      std::reverse_copy(zimn_otstupka_n5.begin(), zimn_otstupka_n5.end(), std::begin(v));
    } break;
    default: {}
  };
  switch(std::abs(zimn)-1) {//выбор номеров добавочных недель
    case 4: {
      w.push_back(32); w.push_back(17); w.push_back(31); w.push_back(30);
    } break;
    case 3: {
      w.push_back(32); w.push_back(31); w.push_back(30);
    } break;
    case 2: {
      w.push_back(32); w.push_back(31);
    } break;
    case 1: {
      w.push_back(32);
    } break;
    default: {}
  };
  v1 = v; w1 = w;//копия для вычислений апостола
  winter_indent = zimn; spring_indent = osen; //сохранение в объекте
  t3 = get_date_(ned8_popashe);//пятидесятницa
  while(true) {//цикл перебора дат всего года
    j = get_dn_(t1);
    //период от начала года до субб.перед нед.омытариифарисеи вкл. без отступки
    //+период от начала года до начала нового ряда чтений при наличии отступки
    if( (zimn!=0 && t1<ddd) || (zimn==0 && t1<dd) ) {
      int k { sn==0 ? sn : (sn>0 ? -sn : std::abs(sn)) };
      set_evangelie_(t1, evangelie_table1_get_chteniya(get_n50_(t1)-k, j));
    }
    //период нового ряда чтений при наличии отступки (до субб.перед нед.омытариифарисеи вкл.)
    if(zimn!=0 && t1>=ddd && t1<dd && j==0) {
      if(!w.empty()) {
        set_evangelie_(t1, evangelie_table1_get_chteniya(w.back(), j));
        w.pop_back();
      }
      if(!v.empty()) v.pop_back();
    }
    if(zimn!=0 && t1>=ddd && t1<dd && j!=0 && !v.empty()) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(v.back(), j));
    }
    //период от нед. о мытари и фарисеи до прощ. воскр. вкл.
    if(t1==dd) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(33, j));
    }
    if(t1>dd && t1<=mf7) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(34, j));
    }
    if(t1>mf7 && t1<=mf14) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(35, j));
    }
    if(t1>mf14 && t1<=mf21) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(36, j));
    }
    //период от начала в.поста до троицкой род.субб вкл.
    if(t1>mf21 && t1<t3) {
      auto fr1 = days.find(t1);
      if(fr1 != days.end()) {
        set_evangelie_(t1, evangelie_table2_get_chteniya(fr1->second.day_markers));
      }
    }
    //период от пятидесятницы до конца года
    if( (t1>=t3 && t1<=dd1) || (t1>dd1 && t1<=ned_po_vozdv && osen>=0) ) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(get_n50_(t1), j));
    }
    if(t1>dd1 && t1<=dd2 && osen<0) {
      if(osen==-2)
        set_evangelie_(t1, evangelie_table1_get_chteniya(osen_otstupka.front(), j));
      else
        set_evangelie_(t1, evangelie_table1_get_chteniya(get_n50_(t1), j));
    }
    if(t1>dd2 && t1<=ned_po_vozdv && osen<0) {
      set_evangelie_(t1, evangelie_table1_get_chteniya(osen_otstupka.back(), j));
    }
    if(t1>ned_po_vozdv) {
      int k { osen==0 ? osen : (osen>0 ? -osen : std::abs(osen)) };
      set_evangelie_(t1, evangelie_table1_get_chteniya(get_n50_(t1)-k, j));
    }
    //конец цикла
    t2 = increment_date_(t1, 1, b);
    if(t2!=t1) { t1 = t2; }
    else       { break; }
  }
  //расчет рядовые чтения апостола на литургии
  t1 = make_pair(1,1);
  while(true) {//цикл перебора дат всего года
    j = get_dn_(t1);
    //период от начала года до субб.перед нед.омытариифарисеи вкл. без отступки
    //+период от начала года до начала нового ряда чтений при наличии отступки
    if( (zimn!=0 && t1<ddd) || (zimn==0 && t1<dd) ) {
      set_apostol_(t1, apostol_table1_get_chteniya(get_n50_(t1), j));
    }
    //период нового ряда чтений при наличии отступки (до субб.перед нед.омытариифарисеи вкл.)
    if(zimn!=0 && t1>=ddd && t1<dd && j==0) {
      if(!w1.empty()) {
        set_apostol_(t1, apostol_table1_get_chteniya(w1.back(), j));
        w1.pop_back();
      }
      if(!v1.empty()) v1.pop_back();
    }
    if(zimn!=0 && t1>=ddd && t1<dd && j!=0 && !v1.empty()) {
      set_apostol_(t1, apostol_table1_get_chteniya(v1.back(), j));
    }
    //период от нед. о мытари и фарисеи до прощ. воскр. вкл.
    if(t1==dd) {
      set_apostol_(t1, apostol_table1_get_chteniya(33, j));
    }
    if(t1>dd && t1<=mf7) {
      set_apostol_(t1, apostol_table1_get_chteniya(34, j));
    }
    if(t1>mf7 && t1<=mf14) {
      set_apostol_(t1, apostol_table1_get_chteniya(35, j));
    }
    if(t1>mf14 && t1<=mf21) {
      set_apostol_(t1, apostol_table1_get_chteniya(36, j));
    }
    //период от начала в.поста до троицкой род.субб вкл.
    if(t1>mf21 && t1<t3) {
      auto fr1 = days.find(t1);
      if(fr1 != days.end()) {
        set_apostol_(t1, apostol_table2_get_chteniya(fr1->second.day_markers));
      }
    }
    //период от пятидесятницы до конца года
    if(t1>=t3) {
      if(!osen_otstupka_apostol) {
        set_apostol_(t1, apostol_table1_get_chteniya(get_n50_(t1), j));
      } else {
        if( (t1>=t3 && t1<=dd1) || (t1>dd1 && t1<=ned_po_vozdv && osen>=0) ) {
          set_apostol_(t1, apostol_table1_get_chteniya(get_n50_(t1), j));
        }
        if(t1>dd1 && t1<=dd2 && osen<0) {
          if(osen==-2)
            set_apostol_(t1, apostol_table1_get_chteniya(osen_otstupka.front(), j));
          else
            set_apostol_(t1, apostol_table1_get_chteniya(get_n50_(t1), j));
        }
        if(t1>dd2 && t1<=ned_po_vozdv && osen<0) {
          set_apostol_(t1, apostol_table1_get_chteniya(osen_otstupka.back(), j));
        }
        if(t1>ned_po_vozdv) {
          int k { osen==0 ? osen : (osen>0 ? -osen : std::abs(osen)) };
          set_apostol_(t1, apostol_table1_get_chteniya(get_n50_(t1)-k, j));
        }
      }
    }
    t2 = increment_date_(t1, 1, b);
    if(t2!=t1) { t1 = t2; }
    else       { break; }
  }
  //save data to object
  data1.reserve(days.size());
  std::for_each(days.begin(), days.end(), [this](const auto& e){
    Data1 d;
    d.dn = e.second.dn;
    d.glas = e.second.glas;
    d.n50 = e.second.n50;
    d.day = e.first.second;
    d.month = e.first.first;
    d.apostol = e.second.apostol;
    d.evangelie = e.second.evangelie;
    std::copy(e.second.day_markers.begin(), e.second.day_markers.end(), d.day_markers.begin());
    data1.push_back(std::move(d));
  });
  data2.reserve(markers.size());
  std::for_each(markers.begin(), markers.end(), [this](const auto& e){
    Data2 d;
    d.marker = e.first;
    d.day = e.second.second;
    d.month = e.second.first;
    data2.push_back(std::move(d));
  });
  data1.shrink_to_fit();
  data2.shrink_to_fit();
}//end OrthYear ctor

int8_t OrthYear::get_date_glas(int8_t month, int8_t day) const
{
  if(auto fr = find_in_data1(month, day); fr) {
    return fr.value()->glas;
  } else {
    return -1;
  }
}

int8_t OrthYear::get_date_n50(int8_t month, int8_t day) const
{
  if(auto fr = find_in_data1(month, day); fr) {
    return fr.value()->n50;
  } else {
    return -1;
  }
}

int8_t OrthYear::get_date_dn(int8_t month, int8_t day) const
{
  if(auto fr = find_in_data1(month, day); fr) {
    return fr.value()->dn;
  } else {
    return -1;
  }
}

ApEvReads OrthYear::get_date_apostol(int8_t month, int8_t day) const
{
  if(auto fr = find_in_data1(month, day); fr) {
    return fr.value()->apostol;
  } else {
    return {};
  }
}

ApEvReads OrthYear::get_date_evangelie(int8_t month, int8_t day) const
{
  if(auto fr = find_in_data1(month, day); fr) {
    return fr.value()->evangelie;
  } else {
    return {};
  }
}

ApEvReads OrthYear::get_resurrect_evangelie(int8_t month, int8_t day) const
{
  auto dn = get_date_dn(month, day);
  if(dn != 0) return {};
  //таблица 11-и воскресныx утрених евангелий
  static const std::array resurrect_evangelie_table = {
    ApEvReads{ 0X742,  "Мф., 116 зач., XXVIII, 16–20." },
    ApEvReads{ 0X463 , "Мк., 70 зач., XVI, 1–8." },
    ApEvReads{ 0X473 , "Мк., 71 зач., XVI, 9–20." },
    ApEvReads{ 0X704,  "Лк., 112 зач., XXIV, 1–12." },
    ApEvReads{ 0X714,  "Лк., 113 зач., XXIV, 12–35." },
    ApEvReads{ 0X724,  "Лк., 114 зач., XXIV, 36–53." },
    ApEvReads{ 0X3f5 , "Ин., 63 зач., XX, 1–10." },
    ApEvReads{ 0X405 , "Ин., 64 зач., XX, 11–18." },
    ApEvReads{ 0X415 , "Ин., 65 зач., XX, 19–31." },
    ApEvReads{ 0X425 , "Ин., 66 зач., XXI, 1–14." },
    ApEvReads{ 0X435 , "Ин., 67 зач., XXI, 15–25." }
  };
  //таблица праздничных утрених евангелий
  static const std::array holydays_evangelie_table = {
    ApEvReads{ 0X532, "Мф., 83 зач., XXI, 1–11, 15–17." },//Вербное воскресенье
    ApEvReads{ 0X23,  "Мк., 2 зач., I, 9–11." },          //Крещение
    ApEvReads{ 0X84,  "Лк., 8 зач., II, 25–32."},         //Сре́тение
    ApEvReads{ 0X44,  "Лк., 4 зач., I, 39–49, 56."},      //Благовещ́ение, Успе́ние, Рождество, Введе́ние Пресв.Богородицы
    ApEvReads{ 0X2d4, "Лк., 45 зач., IX, 28–36."},        //Преображение
    ApEvReads{ 0X2a5, "Ин., 42 зач., XII, 28-36."},       //Воздви́жение
    ApEvReads{ 0X22,  "Мф., 2 зач., I, 18–25."}           //Рождество
  };
  static const std::array unique_evangelie_table = {
    ned2_popashe,
    ned3_popashe,
    ned4_popashe,
    ned5_popashe,
    ned6_popashe,
    ned7_popashe,
    ned8_popashe,
    vel_post_d0n7,
    m1d6,
    sretenie,
    m3d25,
    m8d6,
    m8d15,
    m9d8,
    m9d14,
    m11d21,
    m12d25
  };
  auto w = unique_evangelie_table.end();
  if( auto date_properties = get_date_properties(month, day); date_properties ) {
    w = std::find_first_of(unique_evangelie_table.begin(),
                              unique_evangelie_table.end(),
                              date_properties->begin(),
                              date_properties->end());
  }
  if( w != unique_evangelie_table.end() ) {
    switch(*w) {
      case ned2_popashe:  { return resurrect_evangelie_table[0]; }
      case ned3_popashe:  { return resurrect_evangelie_table[2]; }
      case ned4_popashe:  { return resurrect_evangelie_table[3]; }
      case ned5_popashe:  { return resurrect_evangelie_table[6]; }
      case ned6_popashe:  { return resurrect_evangelie_table[7]; }
      case ned7_popashe:  { return resurrect_evangelie_table[9]; }
      case ned8_popashe:  { return resurrect_evangelie_table[8]; }
      case vel_post_d0n7: { return holydays_evangelie_table[0]; }
      case m1d6:          { return holydays_evangelie_table[1]; }
      case sretenie:      { return holydays_evangelie_table[2]; }
      case m3d25:         { return holydays_evangelie_table[3]; }
      case m8d6:          { return holydays_evangelie_table[4]; }
      case m8d15:         { return holydays_evangelie_table[3]; }
      case m9d8:          { return holydays_evangelie_table[3]; }
      case m9d14:         { return holydays_evangelie_table[5]; }
      case m11d21:        { return holydays_evangelie_table[3]; }
      case m12d25:        { return holydays_evangelie_table[6]; }
      default:            { return {}; }
    };
  } else {
    auto n50 = get_date_n50(month, day);
    if(n50>0 && n50<12) {
      return resurrect_evangelie_table[n50-1] ;
    } else if(n50>11) {
      uint8_t x = n50 % 11;
      if(x==0) x = 10; else x--;
      return resurrect_evangelie_table[x] ;
    }
  }
  return {};
}

std::optional<std::vector<uint16_t>> OrthYear::get_date_properties(int8_t month, int8_t day) const
{
  if(auto fr = find_in_data1(month, day); fr) {
    std::vector<uint16_t> res ;
    std::copy_if(fr.value()->day_markers.begin(), fr.value()->day_markers.end(),
                  std::back_inserter(res),
                  [](auto x){ return x>0; });
    if(res.empty()) return std::nullopt;
    return res;
  } else {
    return std::nullopt;
  }
}

std::optional<ShortDate> OrthYear::get_date_with(uint16_t m) const
{
  if(m<1) return std::nullopt;
  auto fr = std::lower_bound(data2.begin(), data2.end(), m);
  if(fr==data2.end()) return std::nullopt;
  if( !(*fr==m) ) return std::nullopt;
  return ShortDate{fr->month, fr->day};
}

std::optional<std::vector<ShortDate>> OrthYear::get_alldates_with(uint16_t m) const
{
  if(m<1) return std::nullopt;
  auto [begin, end] = std::equal_range(data2.begin(), data2.end(), m);
  if(begin==data2.end()) return std::nullopt;
  std::vector<ShortDate> res ;
  std::transform(begin, end, std::back_inserter(res),
        [](const auto& e){ return ShortDate{e.month, e.day}; });
  if(res.empty()) return std::nullopt;
  return res;
}

std::optional<ShortDate> OrthYear::get_date_withanyof(std::span<oxc_const> m) const
{
  if(m.empty()) return std::nullopt;
  for(auto i: m) { if(auto x = get_date_with(i); x) return *x; }
  return std::nullopt;
}

std::optional<ShortDate> OrthYear::get_date_withallof(std::span<oxc_const> m) const
{
  if(m.empty()) return std::nullopt;
  auto semires = get_alldates_with(m.front());
  if(!semires) return std::nullopt;
  for(auto [month, day] : *semires) {
    const bool b = std::all_of(m.begin(), m.end(), [this, month, day](auto x){
      auto fr = find_in_data1(month, day);
      return std::any_of(fr.value()->day_markers.begin(), fr.value()->day_markers.end(),
                          [x](auto y){ return y==x; });
    });
    if(b) return ShortDate{month, day};
  }
  return std::nullopt;
}

std::optional<std::vector<ShortDate>> OrthYear::get_alldates_withanyof(std::span<oxc_const> m) const
{
  if(m.empty()) return std::nullopt;
  std::vector<ShortDate> result;
  for(auto i: m) {
    if(auto x=get_alldates_with(i); x)
      std::copy(x->begin(), x->end(), std::back_inserter(result));
  }
  if(result.empty()) return std::nullopt;
  else return result;
}

/*----------------------------------------------------*/
/*          class OrthodoxCalendar::impl              */
/*----------------------------------------------------*/

class OrthodoxCalendar::impl {

  //настройка номеров добавочных седмиц зимней отступкu литургийных чтений
  std::array<uint8_t,5> zimn_otstupka_n5; //при отступке в 5 седмиц.
  std::array<uint8_t,4> zimn_otstupka_n4; //при отступке в 4 седмиц.
  std::array<uint8_t,3> zimn_otstupka_n3; //при отступке в 3 седмиц.
  std::array<uint8_t,2> zimn_otstupka_n2; //при отступке в 2 седмиц.
  std::array<uint8_t,1> zimn_otstupka_n1; //при отступке в 1 седмиц.
  //настройка номеров добавочных седмиц осенней отступкu литургийных чтений
  std::array<uint8_t,2> osen_otstupka;
  bool osen_otstupka_apostol; //при вычислении осенней отступкu учитывать ли апостол
  mutable std::unordered_map<std::string, oxc::OrthYear> orthyear_cache;

  OrthYear& get_orthyear_obj(const std::string& year) const;
  template<typename Container>
    bool set_indent_week_numbers_option(Container& container, std::initializer_list<uint8_t> il);
  template<typename MethodPtr>
    auto get_date_option(const Date& date, MethodPtr mptr) const;
  template<typename TProperty, typename OrthYearMethod, typename SelfPeriodMethod>
    Date get_date__(const Year& year, TProperty property, const CalendarFormat infmt, OrthYearMethod orthyear_method,
          SelfPeriodMethod period_method) const;
  template<typename TProperty, typename OrthYearMethod>
    Date get_date_inperiod__(const Date& d1, const Date& d2, TProperty property, OrthYearMethod orthyear_method) const;
  template<typename TProperty, typename OrthYearMethod, typename SelfPeriodMethod>
    std::vector<Date> get_alldates__(const Year& year, TProperty property, const CalendarFormat infmt,
          OrthYearMethod orthyear_method, SelfPeriodMethod period_method) const;
  template<typename TProperty, typename OrthYearMethod>
    std::vector<Date> get_alldates_inperiod__(const Date& d1, const Date& d2, TProperty property,
          OrthYearMethod orthyear_method) const;

public:

  impl();
  bool set_winter_indent_weeks_1(const uint8_t w1);
  bool set_winter_indent_weeks_2(const uint8_t w1, const uint8_t w2);
  bool set_winter_indent_weeks_3(const uint8_t w1, const uint8_t w2, const uint8_t w3);
  bool set_winter_indent_weeks_4(const uint8_t w1, const uint8_t w2, const uint8_t w3, const uint8_t w4);
  bool set_winter_indent_weeks_5(const uint8_t w1, const uint8_t w2, const uint8_t w3, const uint8_t w4,
        const uint8_t w5);
  bool set_spring_indent_weeks(const uint8_t w1, const uint8_t w2);
  void set_spring_indent_apostol(const bool value);
  std::pair<std::vector<uint8_t>, bool> get_options() const;
  std::pair<Month, Day> julian_pascha(const Year& year) const;
  Date pascha(const Year& year, const CalendarFormat infmt) const;
  int8_t winter_indent(const Year& year) const;
  int8_t spring_indent(const Year& year) const;
  int8_t apostol_post_length(const Year& year) const;
  auto date_glas(const Date& d) const;
  auto date_n50(const Date& d) const;
  std::vector<uint16_t> date_properties(const Date& d) const;
  auto date_apostol(const Date& d) const;
  auto date_evangelie(const Date& d) const;
  auto resurrect_evangelie(const Date& d) const;
  bool is_date_of(const Date& d, oxc_const property) const;
  Date get_date_with(const Year& year, oxc_const property, const CalendarFormat infmt) const;
  Date get_date_inperiod_with(const Date& d1, const Date& d2, oxc_const property) const;
  std::vector<Date> get_alldates_with(const Year& year, oxc_const property, const CalendarFormat infmt) const;
  std::vector<Date> get_alldates_inperiod_with(const Date& d1, const Date& d2, oxc_const property) const;
  Date get_date_withanyof(const Year& year, std::span<oxc_const> properties, const CalendarFormat infmt) const;
  Date get_date_inperiod_withanyof(const Date& d1, const Date& d2, std::span<oxc_const> properties) const;
  Date get_date_withallof(const Year& year, std::span<oxc_const> properties, const CalendarFormat infmt) const;
  Date get_date_inperiod_withallof(const Date& d1, const Date& d2, std::span<oxc_const> properties) const;
  std::vector<Date> get_alldates_withanyof(const Year& year, std::span<oxc_const> properties,
        const CalendarFormat infmt) const;
  std::vector<Date> get_alldates_inperiod_withanyof(const Date& d1, const Date& d2,
        std::span<oxc_const> properties) const;
  std::string get_description_for_date(const Date& d, std::string& datefmt) const;
  std::string get_description_for_dates(std::span<const Date> days, std::string& datefmt,
        const std::string& separator) const;
};

OrthodoxCalendar::impl::impl() :
    zimn_otstupka_n5           {30,31,17,32,33},
    zimn_otstupka_n4           {30,31,32,33},
    zimn_otstupka_n3           {31,32,33},
    zimn_otstupka_n2           {32,33},
    zimn_otstupka_n1           {33},
    osen_otstupka              {10,11},
    osen_otstupka_apostol      {false}
{
}

OrthYear& OrthodoxCalendar::impl::get_orthyear_obj(const std::string& year) const
{
  auto [indent_opts, apostol_opt] = get_options();
  std::string indent_opts_str;
  for(const auto x: indent_opts) indent_opts_str += std::to_string(x);
  std::string key (year + indent_opts_str + std::to_string(apostol_opt));
  if(auto x = orthyear_cache.find(key); x != orthyear_cache.end()) {
    return x->second;
  } else {
    if(orthyear_cache.size() == 10000) orthyear_cache.clear();
    auto [it, inserted] = orthyear_cache.try_emplace(key, year, indent_opts, apostol_opt);
    if(!inserted)
      throw std::runtime_error("ошибка создания объекта OrthYear("+year+", "+indent_opts_str+", "
                                +std::to_string(apostol_opt)+")");
    return it->second;
  }
}

template<typename Container>
  bool OrthodoxCalendar::impl::set_indent_week_numbers_option(Container& container, std::initializer_list<uint8_t> il)
{
  if( std::any_of(il.begin(), il.end(), [](auto i){ return i<1 || i>33; }) ) return false;
  if( !std::equal(container.cbegin(), container.cend(), il.begin()) ) {
    std::copy(il.begin(), il.end(), container.begin());
  }
  return true;
}

template<typename MethodPtr>
    auto OrthodoxCalendar::impl::get_date_option(const Date& date, MethodPtr mptr) const
{
  if(!date) throw std::runtime_error(invalid_date);
  const auto& orthyear_obj = get_orthyear_obj(date.year(Julian));
  return (&orthyear_obj->*mptr)(date.month(Julian), date.day(Julian));
}

template<typename TProperty, typename OrthYearMethod, typename SelfPeriodMethod>
  Date OrthodoxCalendar::impl::get_date__(const Year& year, TProperty property, const CalendarFormat infmt,
        OrthYearMethod orthyear_method, SelfPeriodMethod period_method) const
{
  if(infmt==Julian) {
    const auto& orthyear_obj = get_orthyear_obj(year);
    if(auto x = (&orthyear_obj->*orthyear_method)(property); x) {
      return Date (year, x->first, x->second, Julian);
    } else return {};
  } else {
    return (this->*period_method)(Date(year, 1, 1, infmt), Date(year, 12, 31, infmt), property);
  }
}

template<typename TProperty, typename OrthYearMethod>
  Date OrthodoxCalendar::impl::get_date_inperiod__(const Date& d1, const Date& d2, TProperty property,
        OrthYearMethod orthyear_method) const
{
  if(!d1 || !d2) throw std::runtime_error(invalid_date);
  auto [min, max] = std::minmax(d1, d2);
  auto a = string_to_year(min.year(Julian));
  auto b = string_to_year(max.year(Julian)) + 1;
  while(a<b) {
    std::string y = a.str();
    const auto& orthyear_obj = get_orthyear_obj(y);
    if(auto x = (&orthyear_obj->*orthyear_method)(property); x) {
      Date result(y, x->first, x->second, Julian);
      if( result >= min && result <= max ) return result;
    }
    a++;
  }
  return {};
}

template<typename TProperty, typename OrthYearMethod, typename SelfPeriodMethod>
  std::vector<Date> OrthodoxCalendar::impl::get_alldates__(const Year& year, TProperty property,
        const CalendarFormat infmt, OrthYearMethod orthyear_method, SelfPeriodMethod period_method) const
{
  if(infmt==Julian) {
    const auto& orthyear_obj = get_orthyear_obj(year);
    if(auto x = (&orthyear_obj->*orthyear_method)(property); x) {
      std::vector<Date> result;
      result.reserve(x->size()) ;
      std::transform(x->begin(), x->end(), std::back_inserter(result), [&year](const auto& e){
          return Date(year, e.first, e.second, Julian);
      });
      return result;
    }
    else return {};
  } else {
    return (this->*period_method)(Date(year, 1, 1, infmt), Date(year, 12, 31, infmt), property);
  }
}

template<typename TProperty, typename OrthYearMethod>
  std::vector<Date> OrthodoxCalendar::impl::get_alldates_inperiod__(const Date& d1, const Date& d2,
        TProperty property, OrthYearMethod orthyear_method) const
{
  if(!d1 || !d2) throw std::runtime_error(invalid_date);
  std::vector<Date> semiresult, result;
  auto [min, max] = std::minmax(d1, d2);
  auto a = string_to_year(min.year(Julian));
  auto b = string_to_year(max.year(Julian)) + 1;
  while(a<b) {
    std::string y = a.str();
    const auto& orthyear_obj = get_orthyear_obj(y);
    if(auto x = (&orthyear_obj->*orthyear_method)(property); x) {
      std::transform(x->begin(), x->end(), std::back_inserter(semiresult), [&y](const auto& e){
          return Date(y, e.first, e.second, Julian);
      });
    }
    a++;
  }
  if(semiresult.empty()) return {};
  std::sort(semiresult.begin(), semiresult.end());
  auto begin = std::lower_bound(semiresult.begin(), semiresult.end(), min);
  if(begin==semiresult.end()) return {};
  auto end = std::upper_bound(semiresult.begin(), semiresult.end(), max);
  result.reserve(semiresult.size());
  std::copy(begin, end, std::back_inserter(result));
  result.shrink_to_fit();
  return result;
}

bool OrthodoxCalendar::impl::set_winter_indent_weeks_1(const uint8_t w1)
{
  return set_indent_week_numbers_option(zimn_otstupka_n1, {w1});
}

bool OrthodoxCalendar::impl::set_winter_indent_weeks_2(const uint8_t w1, const uint8_t w2)
{
  return set_indent_week_numbers_option(zimn_otstupka_n2, {w1, w2});
}

bool OrthodoxCalendar::impl::set_winter_indent_weeks_3(const uint8_t w1, const uint8_t w2, const uint8_t w3)
{
  return set_indent_week_numbers_option(zimn_otstupka_n3, {w1, w2, w3});
}

bool OrthodoxCalendar::impl::set_winter_indent_weeks_4(const uint8_t w1, const uint8_t w2,
      const uint8_t w3, const uint8_t w4)
{
  return set_indent_week_numbers_option(zimn_otstupka_n4, {w1, w2, w3, w4});
}

bool OrthodoxCalendar::impl::set_winter_indent_weeks_5(const uint8_t w1, const uint8_t w2,
      const uint8_t w3, const uint8_t w4, const uint8_t w5)
{
  return set_indent_week_numbers_option(zimn_otstupka_n5, {w1, w2, w3, w4, w5});
}

bool OrthodoxCalendar::impl::set_spring_indent_weeks(const uint8_t w1, const uint8_t w2)
{
  return set_indent_week_numbers_option(osen_otstupka, {w1, w2});
}

void OrthodoxCalendar::impl::set_spring_indent_apostol(const bool value)
{
  osen_otstupka_apostol = value;
}

std::pair<std::vector<uint8_t>, bool> OrthodoxCalendar::impl::get_options() const
{
  std::vector<uint8_t> first_res(17, 0);
  auto it = first_res.begin();
  it = std::copy(zimn_otstupka_n1.begin(), zimn_otstupka_n1.end(), it);
  it = std::copy(zimn_otstupka_n2.begin(), zimn_otstupka_n2.end(), it);
  it = std::copy(zimn_otstupka_n3.begin(), zimn_otstupka_n3.end(), it);
  it = std::copy(zimn_otstupka_n4.begin(), zimn_otstupka_n4.end(), it);
  it = std::copy(zimn_otstupka_n5.begin(), zimn_otstupka_n5.end(), it);
  it = std::copy(osen_otstupka.begin(), osen_otstupka.end(), it);
  return {first_res, osen_otstupka_apostol};
}

std::pair<Month, Day> OrthodoxCalendar::impl::julian_pascha(const Year& year) const
{
  const auto& orthyear_obj = get_orthyear_obj(year);
  return orthyear_obj.get_date_with(oxc::pasha).value();
}

Date OrthodoxCalendar::impl::pascha(const Year& year, const CalendarFormat infmt) const
{
  return get_date_with(year, oxc::pasha, infmt);
}

int8_t OrthodoxCalendar::impl::winter_indent(const Year& year) const
{
  const auto& orthyear_obj = get_orthyear_obj(year);
  return orthyear_obj.get_winter_indent() ;
}

int8_t OrthodoxCalendar::impl::spring_indent(const Year& year) const
{
  const auto& orthyear_obj = get_orthyear_obj(year);
  return orthyear_obj.get_spring_indent() ;
}

int8_t OrthodoxCalendar::impl::apostol_post_length(const Year& year) const
{
  auto dec_date_by_one = [](Month& m, Day& d, const bool leap)
  {
    d--;
    if(d < 1) {
      m--;
      if(m<1) m = 12;
      d += month_length(m, leap);
    }
  };
  const auto& orthyear_obj = get_orthyear_obj(year);
  auto d1 = orthyear_obj.get_date_with(oxc::ned1_po50);
  auto d2 = orthyear_obj.get_date_with(oxc::m6d29);
  if(d1 && d2) {
    const bool b = is_leap_year(year, Julian);
    int8_t days_count{};
    do {
      dec_date_by_one(d2->first, d2->second, b);
      days_count++;
    } while(*d1 != *d2);
    return days_count-1;
  }
  return 0;
}

auto OrthodoxCalendar::impl::date_glas(const Date& d) const
{
  return get_date_option(d, &OrthYear::get_date_glas);
}

auto OrthodoxCalendar::impl::date_n50(const Date& d) const
{
  return get_date_option(d, &OrthYear::get_date_n50);
}

std::vector<uint16_t> OrthodoxCalendar::impl::date_properties(const Date& date) const
{
  if(!date) return {};
  const auto& orthyear_obj = get_orthyear_obj(date.year(Julian));
  if(auto x = orthyear_obj.get_date_properties(date.month(Julian), date.day(Julian)); x) return x.value();
  else return {};
}

auto OrthodoxCalendar::impl::date_apostol(const Date& d) const
{
  return get_date_option(d, &OrthYear::get_date_apostol);
}

auto OrthodoxCalendar::impl::date_evangelie(const Date& d) const
{
  return get_date_option(d, &OrthYear::get_date_evangelie);
}

auto OrthodoxCalendar::impl::resurrect_evangelie(const Date& d) const
{
  return get_date_option(d, &OrthYear::get_resurrect_evangelie);
}

bool OrthodoxCalendar::impl::is_date_of(const Date& d, oxc_const property) const
{
  if(auto x = date_properties(d); !x.empty()) {
    return std::any_of( x.begin(), x.end(), [property](auto i){ return i==property; } );
  }
  return false;
}

Date OrthodoxCalendar::impl::get_date_with(const Year& year, oxc_const property,
      const CalendarFormat infmt) const
{
  return get_date__(year, property, infmt, &OrthYear::get_date_with, &impl::get_date_inperiod_with);
}

Date OrthodoxCalendar::impl::get_date_inperiod_with(const Date& d1, const Date& d2, oxc_const property) const
{
  return get_date_inperiod__(d1, d2, property, &OrthYear::get_date_with);
}

std::vector<Date> OrthodoxCalendar::impl::get_alldates_with(const Year& year, oxc_const property,
      const CalendarFormat infmt) const
{
  return get_alldates__(year, property, infmt, &OrthYear::get_alldates_with, &impl::get_alldates_inperiod_with);
}

std::vector<Date> OrthodoxCalendar::impl::get_alldates_inperiod_with(const Date& d1, const Date& d2,
      oxc_const property) const
{
  return get_alldates_inperiod__(d1, d2, property, &OrthYear::get_alldates_with);
}

Date OrthodoxCalendar::impl::get_date_withanyof(const Year& year, std::span<oxc_const> properties,
      const CalendarFormat infmt) const
{
  return get_date__(year, properties, infmt, &OrthYear::get_date_withanyof, &impl::get_date_inperiod_withanyof);
}

Date OrthodoxCalendar::impl::get_date_inperiod_withanyof(const Date& d1, const Date& d2,
      std::span<oxc_const> properties) const
{
  return get_date_inperiod__(d1, d2, properties, &OrthYear::get_date_withanyof);
}

Date OrthodoxCalendar::impl::get_date_withallof(const Year& year, std::span<oxc_const> properties,
      const CalendarFormat infmt) const
{
  return get_date__(year, properties, infmt, &OrthYear::get_date_withallof, &impl::get_date_inperiod_withallof);
}

Date OrthodoxCalendar::impl::get_date_inperiod_withallof(const Date& d1, const Date& d2,
      std::span<oxc_const> properties) const
{
  return get_date_inperiod__(d1, d2, properties, &OrthYear::get_date_withallof);
}

std::vector<Date> OrthodoxCalendar::impl::get_alldates_withanyof(const Year& year, std::span<oxc_const> properties,
      const CalendarFormat infmt) const
{
  return get_alldates__(year, properties, infmt, &OrthYear::get_alldates_withanyof,
                                                             &impl::get_alldates_inperiod_withanyof);
}

std::vector<Date> OrthodoxCalendar::impl::get_alldates_inperiod_withanyof(const Date& d1, const Date& d2,
      std::span<oxc_const> properties) const
{
  return get_alldates_inperiod__(d1, d2, properties, &OrthYear::get_alldates_withanyof);
}

std::string OrthodoxCalendar::impl::get_description_for_date(const Date& d, std::string& datefmt) const
{
  if(!d) return {};
  std::string result, buf;
  auto p = date_properties(d);
  for(const auto i: p) if(i < 3001) buf += property_title(i) + ' ';
  if(auto x = std::find(p.begin(), p.end(), oxc::post_petr); x!=p.end())
        buf += property_title(oxc::post_petr) + ". ";
  if(auto x = std::find(p.begin(), p.end(), oxc::post_usp); x!=p.end())
        buf += property_title(oxc::post_usp) + ". ";
  if(auto x = std::find(p.begin(), p.end(), oxc::post_rojd); x!=p.end())
        buf += property_title(oxc::post_rojd) + ". ";
  result = d.format(datefmt) + ' ' + buf;
  while(!result.empty() && result.front()==' ') result.erase(result.begin());
  while(!result.empty() && result.back()==' ') result.pop_back();
  return result;
}

std::string OrthodoxCalendar::impl::get_description_for_dates(std::span<const Date> days, std::string& datefmt,
      const std::string& separator) const
{
  std::string res;
  for(auto it=days.begin(); it!=days.end(); ++it){
    if(auto s = get_description_for_date(*it, datefmt); !s.empty()){
      if(it!=days.begin()) res += separator;
      res += s;
    }
  }
  return res;
}

/*----------------------------------------------*/
/*          class OrthodoxCalendar              */
/*----------------------------------------------*/

OrthodoxCalendar::OrthodoxCalendar() : pimpl(new OrthodoxCalendar::impl())
{
}

OrthodoxCalendar::~OrthodoxCalendar() = default	;

OrthodoxCalendar::OrthodoxCalendar(OrthodoxCalendar&&) noexcept = default;

OrthodoxCalendar& OrthodoxCalendar::operator=(OrthodoxCalendar&&) noexcept = default;

OrthodoxCalendar::OrthodoxCalendar(const OrthodoxCalendar& other) : pimpl(new OrthodoxCalendar::impl(*other.pimpl))
{
}

OrthodoxCalendar& OrthodoxCalendar::operator=(const OrthodoxCalendar& other)
{
  if(this != &other) pimpl.reset(new OrthodoxCalendar::impl(*other.pimpl));
  return *this;
}

bool OrthodoxCalendar::set_winter_indent_weeks_1(const uint8_t w1)
{
  return pimpl->set_winter_indent_weeks_1(w1);
}

bool OrthodoxCalendar::set_winter_indent_weeks_2(const uint8_t w1, const uint8_t w2)
{
  return pimpl->set_winter_indent_weeks_2(w1, w2);
}

bool OrthodoxCalendar::set_winter_indent_weeks_3(const uint8_t w1, const uint8_t w2, const uint8_t w3)
{
  return pimpl->set_winter_indent_weeks_3(w1, w2, w3);
}

bool OrthodoxCalendar::set_winter_indent_weeks_4(const uint8_t w1, const uint8_t w2, const uint8_t w3, const uint8_t w4)
{
  return pimpl->set_winter_indent_weeks_4(w1, w2, w3, w4);
}

bool OrthodoxCalendar::set_winter_indent_weeks_5(const uint8_t w1, const uint8_t w2, const uint8_t w3,
      const uint8_t w4, const uint8_t w5)
{
  return pimpl->set_winter_indent_weeks_5(w1, w2, w3, w4, w5);
}

bool OrthodoxCalendar::set_spring_indent_weeks(const uint8_t w1, const uint8_t w2)
{
  return pimpl->set_spring_indent_weeks(w1, w2);
}

void OrthodoxCalendar::set_spring_indent_apostol(const bool value)
{
  return pimpl->set_spring_indent_apostol(value);
}

std::pair<std::vector<uint8_t>, bool> OrthodoxCalendar::get_options() const
{
  return pimpl->get_options();
}

std::pair<Month, Day> OrthodoxCalendar::julian_pascha(const Year& year) const
{
  return pimpl->julian_pascha(year);
}

Date OrthodoxCalendar::pascha(const Year& year, const CalendarFormat infmt) const
{
  return pimpl->pascha(year, infmt);
}

int8_t OrthodoxCalendar::winter_indent(const Year& year) const
{
  return pimpl->winter_indent(year);
}

int8_t OrthodoxCalendar::spring_indent(const Year& year) const
{
  return pimpl->spring_indent(year);
}

int8_t OrthodoxCalendar::apostol_post_length(const Year& year) const
{
  return pimpl->apostol_post_length(year);
}

int8_t OrthodoxCalendar::date_glas(const Year& y, const Month m, const Day d, const CalendarFormat infmt) const
{
  return pimpl->date_glas(Date(y, m, d, infmt));
}

int8_t OrthodoxCalendar::date_glas(const Date& d) const
{
  return pimpl->date_glas(d);
}

int8_t OrthodoxCalendar::date_n50(const Year& y, const Month m, const Day d, const CalendarFormat infmt) const
{
  return pimpl->date_n50(Date(y, m, d, infmt));
}

int8_t OrthodoxCalendar::date_n50(const Date& d) const
{
  return pimpl->date_n50(d);
}

std::vector<uint16_t> OrthodoxCalendar::date_properties(const Year& y, const Month m, const Day d,
      const CalendarFormat infmt) const
{
  return pimpl->date_properties(Date(y, m, d, infmt));
}

std::vector<uint16_t> OrthodoxCalendar::date_properties(const Date& d) const
{
  return pimpl->date_properties(d);
}

ApEvReads OrthodoxCalendar::date_apostol(const Year& y, const Month m, const Day d, const CalendarFormat infmt) const
{
  return pimpl->date_apostol(Date(y, m, d, infmt));
}

ApEvReads OrthodoxCalendar::date_apostol(const Date& d) const
{
  return pimpl->date_apostol(d);
}

ApEvReads OrthodoxCalendar::date_evangelie(const Year& y, const Month m, const Day d, const CalendarFormat infmt) const
{
  return pimpl->date_evangelie(Date(y, m, d, infmt));
}

ApEvReads OrthodoxCalendar::date_evangelie(const Date& d) const
{
  return pimpl->date_evangelie(d);
}

ApEvReads OrthodoxCalendar::resurrect_evangelie(const Year& y, const Month m, const Day d,
      const CalendarFormat infmt) const
{
  return pimpl->resurrect_evangelie(Date(y, m, d, infmt));
}

ApEvReads OrthodoxCalendar::resurrect_evangelie(const Date& d) const
{
  return pimpl->resurrect_evangelie(d);
}

bool OrthodoxCalendar::is_date_of(const Year& y, const Month m, const Day d, oxc_const property,
      const CalendarFormat infmt) const
{
  return pimpl->is_date_of(Date(y, m, d, infmt), property);
}

bool OrthodoxCalendar::is_date_of(const Date& d, oxc_const property) const
{
  return pimpl->is_date_of(d, property);
}

Date OrthodoxCalendar::get_date_with(const Year& year, oxc_const property, const CalendarFormat infmt) const
{
  return pimpl->get_date_with(year, property, infmt);
}

Date OrthodoxCalendar::get_date_inperiod_with(const Date& d1, const Date& d2, oxc_const property) const
{
  return pimpl->get_date_inperiod_with(d1, d2, property);
}

std::vector<Date> OrthodoxCalendar::get_alldates_with(const Year& year, oxc_const property,
      const CalendarFormat infmt) const
{
  return pimpl->get_alldates_with(year, property, infmt);
}

std::vector<Date> OrthodoxCalendar::get_alldates_inperiod_with(const Date& d1, const Date& d2, oxc_const property) const
{
  return pimpl->get_alldates_inperiod_with(d1, d2, property);
}

Date OrthodoxCalendar::get_date_withanyof(const Year& year, std::span<oxc_const> properties,
      const CalendarFormat infmt) const
{
  return pimpl->get_date_withanyof(year, properties, infmt);
}

Date OrthodoxCalendar::get_date_inperiod_withanyof(const Date& d1, const Date& d2,
      std::span<oxc_const> properties) const
{
  return pimpl->get_date_inperiod_withanyof(d1, d2, properties);
}

Date OrthodoxCalendar::get_date_withallof(const Year& year, std::span<oxc_const> properties,
      const CalendarFormat infmt) const
{
  return pimpl->get_date_withallof(year, properties, infmt);
}

Date OrthodoxCalendar::get_date_inperiod_withallof(const Date& d1, const Date& d2,
      std::span<oxc_const> properties) const
{
  return pimpl->get_date_inperiod_withallof(d1, d2, properties);
}

std::vector<Date> OrthodoxCalendar::get_alldates_withanyof(const Year& year, std::span<oxc_const> properties,
      const CalendarFormat infmt) const
{
  return pimpl->get_alldates_withanyof(year, properties, infmt);
}

std::vector<Date> OrthodoxCalendar::get_alldates_inperiod_withanyof(const Date& d1, const Date& d2,
      std::span<oxc_const> properties) const
{
  return pimpl->get_alldates_inperiod_withanyof(d1, d2, properties);
}

std::string OrthodoxCalendar::get_description_for_date(const Year& y, const Month m, const Day d,
      const CalendarFormat infmt, std::string datefmt) const
{
  return pimpl->get_description_for_date(Date(y, m, d, infmt), datefmt);
}

std::string OrthodoxCalendar::get_description_for_date(const Date& d, std::string datefmt) const
{
  return pimpl->get_description_for_date(d, datefmt);
}

std::string OrthodoxCalendar::get_description_for_dates(std::span<const Date> days, std::string datefmt,
      const std::string separator) const
{
  return pimpl->get_description_for_dates(days, datefmt, separator);
}

} //namespace oxc
