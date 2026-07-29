#include <libpkgcheck-exec/executor.h>
#include <libpkgcheck-exec/error.h>
#include <openssl/evp.h>
#include <array>
#include <memory>
#include <string>
#include <utility>
namespace pkgcheck_exec { namespace {
std::string hash(std::string material){ auto c=std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>(EVP_MD_CTX_new(),EVP_MD_CTX_free); if(!c||EVP_DigestInit_ex(c.get(),EVP_sha256(),nullptr)!=1||EVP_DigestUpdate(c.get(),material.data(),material.size())!=1) throw error(error_code::identity_failed,"cannot hash execution evidence"); std::array<unsigned char,32> b{}; unsigned n=0; if(EVP_DigestFinal_ex(c.get(),b.data(),&n)!=1||n!=32) throw error(error_code::identity_failed,"cannot finalize execution evidence"); static constexpr char h[]="0123456789abcdef"; std::string out(64,'0'); for(unsigned i=0;i<32;++i){out[2*i]=h[b[i]>>4];out[2*i+1]=h[b[i]&15];} return out; }
pkgcheck::check_failure_kind classify(const pkgexec::execution_result& e){ if(e.failure()&&*e.failure()==pkgexec::execution_failure_kind::cancelled) return pkgcheck::check_failure_kind::cancelled; if(e.start_state()==pkgexec::execution_start_state::not_started) return pkgcheck::check_failure_kind::execution_unavailable; return pkgcheck::check_failure_kind::program_failed; }
}
prepared_execution prepare(const admitted_check_session& s){
  using namespace pkgexec;
  std::vector<resource_binding> bindings;
  const auto source_slot=resource_slot::named(resource_role::source_tree,"checked-source"), package_slot=resource_slot::named(resource_role::build_input_tree,"checked-package"), temp_slot=resource_slot::singleton(resource_role::private_temporary_root);
  bindings.emplace_back(source_slot,s.source().tree,resource_access::read_only,logical_path::parse("/check/source"));
  bindings.emplace_back(package_slot,s.package().tree,resource_access::read_only,logical_path::parse("/check/package"));
  bindings.emplace_back(temp_slot,resource_identity::from_sha256(hash("pkgcheck-exec/temp/v1\0"+s.request().identity().hex())),resource_access::writable,logical_path::parse("/tmp"));
  for(const auto& input:s.inputs()) bindings.emplace_back(resource_slot::named(resource_role::check_input_tree,input.input.hex()),input.resource,resource_access::read_only,logical_path::parse("/check/inputs/"+input.input.hex()));
  auto layout=resource_layout::seal(std::move(bindings),package_slot);
  std::vector<environment_variable> vars; vars.emplace_back("ZEPPE_LIN_CHECK_PACKAGE",s.request().check_node().package().name()); vars.emplace_back("ZEPPE_LIN_CHECK_SOURCE","/check/source"); vars.emplace_back("ZEPPE_LIN_CHECK_ROOT","/check/package");
  auto env=environment_policy::hermetic({logical_path::parse("/usr/bin"),logical_path::parse("/bin")},logical_path::parse("/tmp/home"),logical_path::parse("/tmp"),1,0022,std::nullopt,network_policy::denied,stdin_policy::closed,stream_policy::capture_complete,stream_policy::capture_complete,std::move(vars));
  auto request=execution_request::seal(s.request().program(),execution_purpose::check(),s.identity().interpreter,s.paths().root_view,std::move(layout),std::move(env),credential_policy::fixed(s.identity().user_id,s.identity().group_id,s.identity().supplementary_groups,true),s.limits(),cancellation_policy::disabled());
  std::vector<resource_materialization> mats; mats.emplace_back(s.source().tree,s.source().path); mats.emplace_back(s.package().tree,s.package().path); mats.emplace_back(request.resources().binding(temp_slot).resource(),s.paths().temporary_root); for(const auto&i:s.inputs()) mats.emplace_back(i.resource,i.path);
  auto resources=execution_resources::admit(request,s.paths().root_view,s.paths().root_view_path,std::move(mats)); return {std::move(request),std::move(resources)};
}
check_execution_result execute(const admitted_check_session& s,pkgexec::execution_backend& backend){ auto prepared=prepare(s); pkgexec::execution_result evidence=[&]{try{return backend.execute(prepared.request,prepared.resources);}catch(const std::exception& e){throw error(error_code::backend_contract_violation,std::string("execution backend threw instead of returning evidence: ")+e.what());}}(); if(evidence.request()!=prepared.request) throw error(error_code::backend_contract_violation,"execution backend returned evidence for another request"); auto execution=pkgcheck::check_execution_evidence_identity::from_sha256(hash("pkgcheck/execution-evidence/v1\0"+evidence.identity().hex())); pkgcheck::check_result result=evidence.status()==pkgexec::execution_status::succeeded?pkgcheck::check_result::passed(s.request(),execution):pkgcheck::check_result::failed(s.request(),execution,classify(evidence),pkgcheck::check_failure_evidence_identity::from_sha256(hash("pkgcheck/failure-evidence/v1\0"+evidence.identity().hex()))); return check_execution_result(std::move(evidence),std::move(result)); }
}
