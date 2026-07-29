#include <libpkgcheck-exec/error.h>
namespace pkgcheck_exec {
std::string_view to_string(error_code v) noexcept { switch(v){case error_code::invalid_session:return "invalid-session";case error_code::inconsistent_authority:return "inconsistent-authority";case error_code::invalid_path:return "invalid-path";case error_code::duplicate_input:return "duplicate-input";case error_code::missing_input:return "missing-input";case error_code::backend_contract_violation:return "backend-contract-violation";case error_code::identity_failed:return "identity-failed";} return "unknown"; }
error::error(error_code c,std::string m):std::runtime_error(std::move(m)),code_(c){}
error_code error::code()const noexcept{return code_;}
}
