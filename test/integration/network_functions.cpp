#include "network_functions.hpp"
#include <future>

using namespace boost::mysql::test;
using boost::mysql::tcp_prepared_statement;
using boost::mysql::tcp_resultset;
using boost::mysql::tcp_connection;
using boost::mysql::error_info;
using boost::mysql::error_code;
using boost::mysql::detail::make_error_code;
using boost::mysql::errc;
using boost::mysql::value;
using boost::mysql::row;
using boost::mysql::owning_row;

namespace
{

class sync_errc : public network_functions
{
	template <typename Callable>
	static auto impl(Callable&& cb) {
		using R = decltype(cb(std::declval<error_code&>(), std::declval<error_info&>()));
		network_result<R> res;
		res.err = make_error_code(errc::no);
		res.info.set_message("errc info not cleared correctly");
		res.value = cb(res.err, res.info);
		return res;
	}
public:
	const char* name() const override { return "sync_errc"; }
	network_result<no_result> handshake(
		tcp_connection& conn,
		const boost::mysql::connection_params& params
	) override
	{
		return impl([&](error_code& code, error_info& info) {
			conn.handshake(params, code, info);
			return no_result();
		});
	}
	network_result<tcp_resultset> query(
		tcp_connection& conn,
		std::string_view query
	) override
	{
		return impl([&](error_code& code, error_info& info) {
			return conn.query(query, code, info);
		});
	}
	network_result<tcp_prepared_statement> prepare_statement(
		tcp_connection& conn,
		std::string_view statement
	) override
	{
		return impl([&conn, statement](error_code& err, error_info& info) {
			return conn.prepare_statement(statement, err, info);
		});
	}
	network_result<tcp_resultset> execute_statement(
		tcp_prepared_statement& stmt,
		value_list_it params_first,
		value_list_it params_last
	) override
	{
		return impl([=, &stmt](error_code& err, error_info& info) {
			return stmt.execute(params_first, params_last, err, info);
		});
	}
	network_result<tcp_resultset> execute_statement(
		tcp_prepared_statement& stmt,
		const std::vector<value>& values
	) override
	{
		return impl([&stmt, &values](error_code& err, error_info& info) {
			return stmt.execute(values, err, info);
		});
	}
	network_result<no_result> close_statement(
		tcp_prepared_statement& stmt
	) override
	{
		return impl([&](error_code& code, error_info& info) {
			stmt.close(code, info);
			return no_result();
		});
	}
	network_result<const row*> fetch_one(
		tcp_resultset& r
	) override
	{
		return impl([&](error_code& code, error_info& info) {
			return r.fetch_one(code, info);
		});
	}
	network_result<std::vector<owning_row>> fetch_many(
		tcp_resultset& r,
		std::size_t count
	) override
	{
		return impl([&](error_code& code, error_info& info) {
			return r.fetch_many(count, code, info);
		});
	}
	network_result<std::vector<owning_row>> fetch_all(
		tcp_resultset& r
	) override
	{
		return impl([&](error_code& code, error_info& info) {
			return r.fetch_all(code, info);
		});
	}
};

class sync_exc : public network_functions
{
	template <typename Callable>
	static auto impl(Callable&& cb) {
		using R = decltype(cb());
		network_result<R> res;
		try
		{
			res.value = cb();
		}
		catch (const boost::system::system_error& err)
		{
			res.err = err.code();
			res.info.set_message(err.what());
		}
		return res;
	}
public:
	const char* name() const override { return "sync_exc"; }
	network_result<no_result> handshake(
		tcp_connection& conn,
		const boost::mysql::connection_params& params
	) override
	{
		return impl([&] {
			conn.handshake(params);
			return no_result();
		});
	}
	network_result<tcp_resultset> query(
		tcp_connection& conn,
		std::string_view query
	) override
	{
		return impl([&] {
			return conn.query(query);
		});
	}
	network_result<tcp_prepared_statement> prepare_statement(
		tcp_connection& conn,
		std::string_view statement
	) override
	{
		return impl([&conn, statement] {
			return conn.prepare_statement(statement);
		});
	}
	network_result<tcp_resultset> execute_statement(
		tcp_prepared_statement& stmt,
		value_list_it params_first,
		value_list_it params_last
	) override
	{
		return impl([&]{
			return stmt.execute(params_first, params_last);
		});
	}
	network_result<tcp_resultset> execute_statement(
		tcp_prepared_statement& stmt,
		const std::vector<value>& values
	) override
	{
		return impl([&stmt, &values] {
			return stmt.execute(values);
		});
	}
	network_result<no_result> close_statement(
		tcp_prepared_statement& stmt
	) override
	{
		return impl([&] {
			stmt.close();
			return no_result();
		});
	}
	network_result<const row*> fetch_one(
		tcp_resultset& r
	) override
	{
		return impl([&] {
			return r.fetch_one();
		});
	}
	network_result<std::vector<owning_row>> fetch_many(
		tcp_resultset& r,
		std::size_t count
	) override
	{
		return impl([&] {
			return r.fetch_many(count);
		});
	}
	network_result<std::vector<owning_row>> fetch_all(
		tcp_resultset& r
	) override
	{
		return impl([&] {
			return r.fetch_all();
		});
	}
};

class async : public network_functions
{
	template <typename R, typename Callable>
	static network_result<R> impl(Callable&& cb) {
		std::promise<network_result<R>> prom;
		cb([&prom](error_code code, error_info info, auto retval) {
			prom.set_value(network_result<R>{code, std::move(info), std::move(retval)});
		});
		return prom.get_future().get();
	}

	template <typename Callable>
	static network_result<no_result> impl_no_result(Callable&& cb) {
		std::promise<network_result<no_result>> prom;
		cb([&prom](error_code code, error_info info) {
			prom.set_value(network_result<no_result>{code, std::move(info), no_result()});
		});
		return prom.get_future().get();
	}
public:
	const char* name() const override { return "async"; }
	network_result<no_result> handshake(
		tcp_connection& conn,
		const boost::mysql::connection_params& params
	) override
	{
		return impl_no_result([&](auto&& token) {
			return conn.async_handshake(params, std::forward<decltype(token)>(token));
		});
	}
	network_result<tcp_resultset> query(
		tcp_connection& conn,
		std::string_view query
	) override
	{
		return impl<tcp_resultset>([&](auto&& token) {
			return conn.async_query(query, std::forward<decltype(token)>(token));
		});
	}
	network_result<tcp_prepared_statement> prepare_statement(
		tcp_connection& conn,
		std::string_view statement
	) override
	{
		return impl<tcp_prepared_statement>([&conn, statement](auto&& token) {
			return conn.async_prepare_statement(statement, std::forward<decltype(token)>(token));
		});
	}
	network_result<tcp_resultset> execute_statement(
		tcp_prepared_statement& stmt,
		value_list_it params_first,
		value_list_it params_last
	) override
	{
		return impl<tcp_resultset>([&](auto&& token) {
			return stmt.async_execute(params_first, params_last, std::forward<decltype(token)>(token));
		});
	}
	network_result<tcp_resultset> execute_statement(
		tcp_prepared_statement& stmt,
		const std::vector<value>& values
	) override
	{
		return impl<tcp_resultset>([&](auto&& token) {
			return stmt.async_execute(values, std::forward<decltype(token)>(token));
		});
	}
	network_result<no_result> close_statement(
		tcp_prepared_statement& stmt
	) override
	{
		return impl_no_result([&](auto&& token) {
			return stmt.async_close(std::forward<decltype(token)>(token));
		});
	}
	network_result<const row*> fetch_one(
		tcp_resultset& r
	) override
	{
		return impl<const row*>([&](auto&& token) {
			return r.async_fetch_one(std::forward<decltype(token)>(token));
		});
	}
	network_result<std::vector<owning_row>> fetch_many(
		tcp_resultset& r,
		std::size_t count
	) override
	{
		return impl<std::vector<owning_row>>([&](auto&& token) {
			return r.async_fetch_many(count, std::forward<decltype(token)>(token));
		});
	}
	network_result<std::vector<owning_row>> fetch_all(
		tcp_resultset& r
	) override
	{
		return impl<std::vector<owning_row>>([&](auto&& token) {
			return r.async_fetch_all(std::forward<decltype(token)>(token));
		});
	}
};

// Global objects to be exposed
sync_errc sync_errc_obj;
sync_exc sync_exc_obj;
async async_obj;

}

// Visible stuff
boost::mysql::test::network_functions* boost::mysql::test::sync_errc_network_functions = &sync_errc_obj;
boost::mysql::test::network_functions* boost::mysql::test::sync_exc_network_functions = &sync_exc_obj;
boost::mysql::test::network_functions* boost::mysql::test::async_network_functions = &async_obj;
