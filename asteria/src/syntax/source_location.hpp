// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_
#define ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Source_Location
  {
  private:
    Cow_String m_file;
    std::uint32_t m_line;

  public:
    Source_Location(Cow_String xfile, std::uint32_t xline) noexcept
      : m_file(rocket::move(xfile)), m_line(xline)
      {
      }

  public:
    const Cow_String& file() const noexcept
      {
        return this->m_file;
      }
    std::uint32_t line() const noexcept
      {
        return this->m_line;
      }

    void print(std::ostream& os) const;
  };

inline std::ostream& operator<<(std::ostream& os, const Source_Location& sloc)
  {
    sloc.print(os);
    return os;
  }

}  // namespace Asteria

#endif
