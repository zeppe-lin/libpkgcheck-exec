#include <libpkgcheck-exec/model.h>
#include <libpkgcheck-exec/error.h>
#include <algorithm>
#include <set>
#include <utility>
namespace pkgcheck_exec { namespace fs=std::filesystem;
namespace {
fs::path absolute(fs::path p,const char* name){ if(p.empty()||!p.is_absolute()) throw error(error_code::invalid_path,std::string(name)+" must be absolute"); return p.lexically_normal(); }
}
admitted_check_session::admitted_check_session(pkgcheck::check_request r,source_tree s,checked_package_tree p,std::vector<package_input_tree> i,session_paths paths,execution_identity x,pkgexec::resource_limits l):request_(std::move(r)),source_(std::move(s)),package_(std::move(p)),inputs_(std::move(i)),paths_(std::move(paths)),identity_(std::move(x)),limits_(std::move(l)){}
admitted_check_session admitted_check_session::admit(pkgcheck::check_request request,source_tree source,checked_package_tree package,std::vector<package_input_tree> inputs,session_paths paths,execution_identity identity,pkgexec::resource_limits limits){
  source.path=absolute(std::move(source.path),"source tree"); package.path=absolute(std::move(package.path),"checked package tree"); paths.root_view_path=absolute(std::move(paths.root_view_path),"root view"); paths.temporary_root=absolute(std::move(paths.temporary_root),"temporary root");
  if(source.source!=request.build().request().source().identity()) throw error(error_code::inconsistent_authority,"source tree does not name the checked source snapshot");
  if(!request.build().artifact()||package.artifact!=request.build().artifact()->identity()) throw error(error_code::inconsistent_authority,"checked package tree does not name the successful build artifact");
  std::sort(inputs.begin(),inputs.end(),[](const auto&a,const auto&b){return a.input<b.input;});
  for(std::size_t n=1;n<inputs.size();++n) if(inputs[n-1].input==inputs[n].input) throw error(error_code::duplicate_input,"duplicate check input tree");
  const auto& expected=request.inputs().inputs();
  if(inputs.size()!=expected.size()) throw error(error_code::missing_input,"check input tree set is incomplete");
  for(std::size_t n=0;n<inputs.size();++n){ inputs[n].path=absolute(std::move(inputs[n].path),"check input tree"); if(inputs[n].input!=expected[n].resolved().identity()||inputs[n].tree!=expected[n].tree()) throw error(error_code::inconsistent_authority,"check input tree does not match the sealed request"); }
  return admitted_check_session(std::move(request),std::move(source),std::move(package),std::move(inputs),std::move(paths),std::move(identity),std::move(limits));
}
const pkgcheck::check_request& admitted_check_session::request()const noexcept{return request_;} const source_tree& admitted_check_session::source()const noexcept{return source_;} const checked_package_tree& admitted_check_session::package()const noexcept{return package_;} const std::vector<package_input_tree>& admitted_check_session::inputs()const noexcept{return inputs_;} const session_paths& admitted_check_session::paths()const noexcept{return paths_;} const execution_identity& admitted_check_session::identity()const noexcept{return identity_;} const pkgexec::resource_limits& admitted_check_session::limits()const noexcept{return limits_;}
check_execution_result::check_execution_result(pkgexec::execution_result e,pkgcheck::check_result c):execution_(std::move(e)),check_(std::move(c)){} const pkgexec::execution_result& check_execution_result::execution()const noexcept{return execution_;} const pkgcheck::check_result& check_execution_result::check()const noexcept{return check_;}
}
