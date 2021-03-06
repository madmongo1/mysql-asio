#ifndef MYSQL_ASIO_IMPL_AUTH_HPP
#define MYSQL_ASIO_IMPL_AUTH_HPP

#include <cstdint>
#include <string_view>
#include <array>

namespace boost {
namespace mysql {
namespace detail {
namespace mysql_native_password {

constexpr const char* plugin_name = "mysql_native_password";
constexpr std::size_t challenge_length = 20;
constexpr std::size_t response_length = 20;

// challenge must point to challenge_length bytes of data
// output must point to response_length bytes of data
inline void compute_auth_string(std::string_view password, const void* challenge, void* output);


} // mysql_native_password
} // detail
} // mysql
} // boost

#include "boost/mysql/detail/auth/impl/mysql_native_password.ipp"

#endif
