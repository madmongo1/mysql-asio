#ifndef INCLUDE_BOOST_MYSQL_DETAIL_NETWORK_ALGORITHMS_IMPL_EXECUTE_STATEMENT_HPP_
#define INCLUDE_BOOST_MYSQL_DETAIL_NETWORK_ALGORITHMS_IMPL_EXECUTE_STATEMENT_HPP_

#include "boost/mysql/detail/network_algorithms/execute_generic.hpp"
#include "boost/mysql/detail/protocol/binary_deserialization.hpp"
#include "boost/mysql/detail/protocol/prepared_statement_messages.hpp"

namespace boost {
namespace mysql {
namespace detail {

template <typename ForwardIterator>
com_stmt_execute_packet<ForwardIterator> make_stmt_execute_packet(
	std::uint32_t statement_id,
	ForwardIterator params_begin,
	ForwardIterator params_end
)
{
	return com_stmt_execute_packet<ForwardIterator> {
		int4(statement_id),
		int1(0), // flags
		int4(1), // iteration count
		int1(1), // new params flag: set
		params_begin,
		params_end
	};
}

} // detail
} // mysql
} // boost

template <typename StreamType, typename ForwardIterator>
void boost::mysql::detail::execute_statement(
	channel<StreamType>& chan,
	std::uint32_t statement_id,
	ForwardIterator params_begin,
	ForwardIterator params_end,
	resultset<StreamType>& output,
	error_code& err,
	error_info& info
)
{
	auto request = make_stmt_execute_packet(statement_id, params_begin, params_end);
	execute_generic(&deserialize_binary_row, chan, request, output, err, info);
}

template <typename StreamType, typename ForwardIterator, typename CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(
	CompletionToken,
	void(boost::mysql::error_code, boost::mysql::error_info, boost::mysql::resultset<StreamType>)
)
boost::mysql::detail::async_execute_statement(
	channel<StreamType>& chan,
	std::uint32_t statement_id,
	ForwardIterator params_begin,
	ForwardIterator params_end,
	CompletionToken&& token
)
{
	auto request = make_stmt_execute_packet(statement_id, params_begin, params_end);
	return async_execute_generic(&deserialize_binary_row, chan, request, std::forward<CompletionToken>(token));
}



#endif /* INCLUDE_BOOST_MYSQL_DETAIL_NETWORK_ALGORITHMS_IMPL_EXECUTE_STATEMENT_HPP_ */
