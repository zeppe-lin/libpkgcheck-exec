#include <libpkgcheck-exec/libpkgcheck-exec.h>
#include "check_fixture.h"
#include "test.h"
#include <algorithm>
#include <string>
#include <vector>
namespace {
std::string hex(char c){return std::string(64,c);}
std::vector<pkgexec::execution_guarantee> guarantees(){return {
 pkgexec::execution_guarantee::exact_interpreter,pkgexec::execution_guarantee::closed_environment,pkgexec::execution_guarantee::root_view,pkgexec::execution_guarantee::read_only_resources,pkgexec::execution_guarantee::writable_resources,pkgexec::execution_guarantee::fixed_credentials,pkgexec::execution_guarantee::network_denied,pkgexec::execution_guarantee::loopback_isolated,pkgexec::execution_guarantee::resource_limits,pkgexec::execution_guarantee::cancellation,pkgexec::execution_guarantee::complete_stdout_capture,pkgexec::execution_guarantee::complete_stderr_capture,pkgexec::execution_guarantee::cleanup_verified,pkgexec::execution_guarantee::cpu_time_limit,pkgexec::execution_guarantee::address_space_limit,pkgexec::execution_guarantee::file_size_limit,pkgexec::execution_guarantee::open_files_limit,pkgexec::execution_guarantee::process_count_limit};}
class backend final:public pkgexec::execution_backend{public: explicit backend(bool pass):pass_(pass){} pkgexec::backend_capability_profile capabilities()const override{return pkgexec::backend_capability_profile::seal(pkgexec::backend_identity::from_sha256(hex('9')),guarantees());} pkgexec::execution_result execute(const pkgexec::execution_request&r,const pkgexec::execution_resources&)override{auto cap=capabilities(); if(pass_) return pkgexec::execution_result::succeeded(r,std::move(cap),r.interpreter(),pkgexec::stream_capture::retained("ok\n"),pkgexec::stream_capture::retained(""),r.required_guarantees()); return pkgexec::execution_result::failed_before_start(r,std::move(cap),pkgexec::execution_failure_kind::interpreter_unavailable,{},"missing");}private:bool pass_;};
pkgcheck_exec::admitted_check_session session(){
  auto scenario=check_fixture::make_scenario();
  auto build=check_fixture::successful_build(scenario.checked,scenario.tester);
  auto r=pkgcheck::check_request::seal(scenario.transaction,check_fixture::node(scenario.transaction,pkgtransaction::transaction_action_kind::check).identity(),build);
  std::vector<pkgcheck_exec::package_input_tree> inputs; char seed='d';
  for(const auto&i:r.inputs().inputs()) inputs.push_back({i.resolved().identity(),i.tree(),pkgexec::resource_identity::from_sha256(hex(seed++)),"/trees/input"});
  return pkgcheck_exec::admitted_check_session::admit(std::move(r),{scenario.checked.identity(),pkgexec::resource_identity::from_sha256(hex('a')),"/trees/source"},{build.artifact()->identity(),pkgexec::resource_identity::from_sha256(hex('b')),"/trees/package"},std::move(inputs),{pkgexec::root_view_identity::from_sha256(hex('c')),"/","/tmp/check-session"},{pkgexec::interpreter_identity::from_sha256(hex('e')),1000,1000,{}});
}
}
int main(){try{
  auto s=session(); auto p=pkgcheck_exec::prepare(s);
  TEST_CHECK(p.request.purpose().kind()==pkgexec::execution_purpose_kind::check);
  TEST_CHECK(p.request.program()==s.request().program());
  backend ok(true); auto passed=pkgcheck_exec::execute(s,ok);
  TEST_CHECK(passed.check().outcome()==pkgcheck::check_outcome::passed);
  backend no(false); auto failed=pkgcheck_exec::execute(s,no);
  TEST_CHECK(failed.check().failure()==pkgcheck::check_failure_kind::execution_unavailable);
  bool rejected=false;
  try {
    auto scenario=check_fixture::make_scenario();
    auto build=check_fixture::successful_build(scenario.checked,scenario.tester);
    auto request=pkgcheck::check_request::seal(scenario.transaction,check_fixture::node(scenario.transaction,pkgtransaction::transaction_action_kind::check).identity(),build);
    std::vector<pkgcheck_exec::package_input_tree> inputs;
    for(const auto&i:request.inputs().inputs()) inputs.push_back({i.resolved().identity(),i.tree(),pkgexec::resource_identity::from_sha256(hex('f')),"/trees/input"});
    (void)pkgcheck_exec::admitted_check_session::admit(std::move(request),{pkgsource::source_snapshot_identity::from_sha256(hex('0')),pkgexec::resource_identity::from_sha256(hex('a')),"/trees/source"},{build.artifact()->identity(),pkgexec::resource_identity::from_sha256(hex('b')),"/trees/package"},std::move(inputs),{pkgexec::root_view_identity::from_sha256(hex('c')),"/","/tmp/check-session"},{pkgexec::interpreter_identity::from_sha256(hex('e')),1000,1000,{}});
  } catch(const pkgcheck_exec::error& e) { rejected=e.code()==pkgcheck_exec::error_code::inconsistent_authority; }
  TEST_CHECK(rejected);
  return 0;
}catch(const std::exception&e){std::cerr<<"unexpected exception: "<<e.what()<<'\n';return 1;}}
