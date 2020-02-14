#ifndef TEST_INTEGRATION_NETWORK_FUNCTIONS_HPP_
#define TEST_INTEGRATION_NETWORK_FUNCTIONS_HPP_

#include "mysql/connection.hpp"
#include "test_common.hpp"
#include <gtest/gtest.h>
#include <forward_list>

namespace mysql
{
namespace test
{

template <typename T>
struct network_result
{
	error_code err;
	error_info info;
	T value;

	void validate_no_error() const
	{
		ASSERT_EQ(err, error_code());
		EXPECT_EQ(info, error_info());
	}

	void validate_error(
		error_code expected_errc,
		const std::vector<std::string>& expected_msg
	) const
	{
		EXPECT_EQ(err, expected_errc);
		validate_string_contains(info.message(), expected_msg);
	}

	void validate_error(
		Error expected_errc,
		const std::vector<std::string>& expected_msg
	) const
	{
		validate_error(detail::make_error_code(expected_errc), expected_msg);
	}
};

using value_list_it = std::forward_list<value>::const_iterator;

class network_functions
{
public:
	virtual ~network_functions() = default;
	virtual network_result<tcp_prepared_statement> prepare_statement(
			tcp_connection&, std::string_view statement) = 0;
	virtual network_result<tcp_resultset> execute_statement(
			tcp_prepared_statement&, value_list_it params_first, value_list_it params_last) = 0;
	virtual network_result<tcp_resultset> execute_statement(
			tcp_prepared_statement&, const std::vector<value>&) = 0;
};

extern network_functions* sync_errc_network_functions;
extern network_functions* sync_exc_network_functions;
extern network_functions* async_network_functions;

inline network_functions* all_network_functions [] = {
	sync_errc_network_functions,
	sync_exc_network_functions,
	async_network_functions
};

#define MYSQL_NETWORK_TEST_SUITE(TestSuiteName) \
	INSTANTIATE_TEST_SUITE_P(Default, TestSuiteName, testing::ValuesIn( \
		all_network_functions \
	), test_name_generator)

}
}



#endif /* TEST_INTEGRATION_NETWORK_FUNCTIONS_HPP_ */