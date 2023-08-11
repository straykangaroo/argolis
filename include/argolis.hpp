#ifndef ARGOLIS_HPP
#define ARGOLIS_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>


namespace argolis {

enum class Arg_Policy { NO_ARG, MAYBE_ARG, EXPECT_ARG };


class Opt_Spec {

public:

    using Action = std::function<void(const Opt_Spec &, std::optional<std::string_view>)>;

    Opt_Spec(char short_name, const std::string & long_name, Arg_Policy arg_policy, const Action & action);
    Opt_Spec(char short_name, Arg_Policy arg_policy, const Action & action);

    char short_name() const;
    const std::string & long_name() const;
    Arg_Policy arg_policy() const;

    void action(const std::optional<std::string_view> & value = std::nullopt) const;

private:
    char _short_name;
    std::string _long_name;
    Arg_Policy _arg_policy;
    Action _action;
};


struct Error {
    enum class Type { BAD_OPT, MISSING_ARG, UNEXPECTED_ARG };
    Type type;
    std::string_view bad_item;
};


class Parser {

public:
    using Arg_Handler = std::function<void(std::string_view)>;
    using Error_Handler = std::function<void(Error)>;

    Parser(std::initializer_list<Opt_Spec> spec_list = {});

    // settings
    void add_opt(const Opt_Spec & spec);
    void combi_allowed(bool allowed);
    void abort_on_error(bool abort);

    // callbacks
    void on_arg(const Arg_Handler & handler);
    void on_err(const Error_Handler & handler);

    void parse(int argc, char * argv[]) const;

private:
    std::vector<Opt_Spec> _specs;
    bool _combi_allowed;
    bool _abort_on_error;

    Arg_Handler _on_arg;            // free-standing argument callback
    Error_Handler _on_err;          // error callback

    std::optional<decltype(_specs)::const_iterator> find_spec(std::string_view name) const;

    // tell if a string is an argument (i.e. first char is not '-', unless the string is exactly == "-")
    static bool is_arg(std::string_view s);

    // types of item we could meet on the command line
    struct Single_Item  { std::string_view name;    };                                  // "-a", "-all", "--all", "-v", "-verbose", "--verbose"
    struct Combi_Item   { std::string_view name;    };                                  // "-av"
    struct Full_Item    { std::string_view name; std::string_view value;    };          // "-num=37", "--num=37"
    struct EOO_Item     {};                                                             // "--"
    struct Arg_Item     {};                                                             // anything else
    using Item = std::variant<Single_Item,Combi_Item,Full_Item,EOO_Item,Arg_Item>;

    // parse/classify an item from the command line
    Item parse_item(std::string_view s) const;

    // given an option name and (optional value), take the appropriate action (i.e. invok eaction callback or error callback)
    std::optional<Error::Type> eval(std::string_view name, const std::optional<std::string_view> & value = std::nullopt) const;
};

// ----------- private

namespace details {

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}   // namespace argolis::details


// ----------- Opt_Spec - implementation

inline Opt_Spec::Opt_Spec(char short_name, const std::string & long_name, Arg_Policy arg_policy, const Action & action)
    : _short_name(short_name), _long_name(long_name), _arg_policy(arg_policy), _action(action)
{
    assert( action );
}

inline Opt_Spec::Opt_Spec(char short_name, Arg_Policy arg_policy, const Action & action)
    : Opt_Spec(short_name,std::string{},arg_policy,action)
{
    assert( action );
}

inline char Opt_Spec::short_name() const
{
    return _short_name;
}

inline const std::string & Opt_Spec::long_name() const
{
    return _long_name;
}

inline Arg_Policy Opt_Spec::arg_policy() const
{
    return _arg_policy;
}

inline void Opt_Spec::action(const std::optional<std::string_view> & value) const
{
    _action(*this,value);
}


// ----------- Parser - implementation

inline Parser::Parser(std::initializer_list<Opt_Spec> spec_list)
    : _specs(spec_list), _combi_allowed(false), _abort_on_error(false), _on_arg([](auto){}), _on_err([](auto){})
{
}

inline void Parser::add_opt(const Opt_Spec & spec)
{
    _specs.push_back(spec);
}

inline void Parser::combi_allowed(bool allowed)
{
    _combi_allowed = allowed;
}

inline void Parser::abort_on_error(bool abort)
{
    _abort_on_error = abort;
}

inline void Parser::on_arg(const Arg_Handler & handler)
{
    assert(handler);

    _on_arg = handler;
}

inline void Parser::on_err(const Error_Handler & handler)
{
    assert(handler);

    _on_err = handler;
}

inline void Parser::parse(int argc, char * argv[]) const
{
    assert( argc > 0 );

    auto p = argv + 1;
    const auto end = argv + argc;
    bool opt_loop = true;

    // p < end instead of p != end because we might increment p by 2, risking so to jump after end
    while( opt_loop && p < end ) {
        p = std::visit(details::overloaded{
            [this, & p, & end](Single_Item item) -> auto
            {
                const bool next_is_arg = (p + 1 != end) && is_arg(*(p+1));
                const auto next = next_is_arg ? std::make_optional(*(p + 1)) : std::nullopt;
                const auto maybe_err_type = eval(item.name,next);

                if( maybe_err_type ) {
                    _on_err({*maybe_err_type,*p});
                    if( _abort_on_error ) {
                        return end;
                    }
                }
                return p + (next_is_arg ? 2 : 1);               // error or not, if the option is followed by an argument we shall skip it too
            },
            [this, & p, & end](Combi_Item item) -> auto
            {
                assert( item.name.length() > 0 );               // assert this so to not foul up the loop below. However, it should not happen, because
                                                                // "-" would be classified as an Arg_Item and "-=x" as a Full_Item

                for( std::string_view::size_type i = 0; i != item.name.length() - 1; i++ ) {
                    const auto maybe_err_type = eval(item.name.substr(i,1));
                    if( maybe_err_type ) {
                        assert( *maybe_err_type == Error::Type::BAD_OPT || *maybe_err_type == Error::Type::MISSING_ARG );
                        _on_err({*maybe_err_type,*p});
                        if( _abort_on_error ) {
                            return end;
                        }
                    }
                }

                const bool next_is_arg = (p + 1 != end) && is_arg(*(p+1));
                const auto next = next_is_arg ? std::make_optional(*(p + 1)) : std::nullopt;
                const auto maybe_err_type = eval(item.name.substr(item.name.length()-1),next);

                if( maybe_err_type ) {
                    _on_err({*maybe_err_type,*p});
                    if( _abort_on_error ) {
                        return end;
                    }
                }
                return p +  (next_is_arg ? 2 : 1);              // error or not, if the option is followed by an argument we shall skip it too
            },
            [this, & p, & end](Full_Item item) -> auto
            {
                const auto maybe_err_type = eval(item.name,item.value);
                if( maybe_err_type ) {
                    _on_err({*maybe_err_type,*p});
                    if( _abort_on_error ) {
                        return end;
                    }
                }
                return p + 1;
            },
            [& opt_loop, & p](EOO_Item) -> auto
            {
                opt_loop = false;
                return p + 1;
            },
            [& opt_loop, & p](Arg_Item) -> auto
            {
                opt_loop = false;
                return p;
            }
        },parse_item(*p));

    }

    // loop the remaining free-standing arguments
    // p < end instead of p != end because we might have incremented p by 2, risking so to jump after end
    while( p < end ) {
        _on_arg(*p++);
    }

}

inline std::optional<decltype(Parser::_specs)::const_iterator> Parser::find_spec(std::string_view name) const
{
    assert( ! name.empty() );

    const auto it = (   name.length() == 1 ?
                        std::find_if(_specs.begin(),_specs.end(),[& name](const Opt_Spec & spec){ return spec.short_name() == name[0];  }) :
                        std::find_if(_specs.begin(),_specs.end(),[& name](const Opt_Spec & spec){ return spec.long_name() == name;      }) );
        
    return (it != _specs.end() ? std::make_optional(it) : std::nullopt);
}

inline bool Parser::is_arg(std::string_view s)
{
    return ( s.length() < 2 || s[0] != '-' );
}

inline Parser::Item Parser::parse_item(std::string_view s) const
{
    if( s.length() < 2 || s[0] != '-' ) {
        return Arg_Item{};
    }

    if( s.length() == 2 ) {
        assert( s[0] == '-' );
        // eoo or single "-x"
        return s[1] == '-' ? Item{EOO_Item{}} : Single_Item{s.substr(1)};
    }

    // combi "-abc" or single "-all" or single "-all" or full "-num=x" or full "--num=x"
    assert( s.length() > 2 && s[0] == '-' );
    const std::string_view::size_type base_pos = (s[1] == '-' ? 2 : 1);             // first char after the dash or double-dash
    if ( base_pos == 1 && _combi_allowed ) {
        // combi "-abc"
        return Combi_Item{s.substr(base_pos)};
    }

    // single "-all" or single "-all" or full "-num=x" or full "--num=x"
    assert( base_pos == 2 || ! _combi_allowed );
    const auto eq_pos = s.find('=');
    if( eq_pos == std::string_view::npos ) {
        // single "-all" or single "-all" 
        return Single_Item{s.substr(base_pos)};
    }

    // full "-num=x" or full "--num=x"
    return Full_Item{s.substr(base_pos,eq_pos-base_pos),s.substr(eq_pos+1)};
}

inline std::optional<Error::Type> Parser::eval(std::string_view name, const std::optional<std::string_view> & value) const
{
    if( name.empty() ) {
        return Error::Type::BAD_OPT;
    }

    const auto maybe_opt_it = find_spec(name);
    if( ! maybe_opt_it ) {
        return Error::Type::BAD_OPT;
    }

    if( (*maybe_opt_it)->arg_policy() == Arg_Policy::EXPECT_ARG && ! value ) {
        return Error::Type::MISSING_ARG;
    }

    if( (*maybe_opt_it)->arg_policy() == Arg_Policy::NO_ARG && value ) {
        return Error::Type::UNEXPECTED_ARG;
    }

    (*maybe_opt_it)->action(value);
    return {};
}

}   // namespace argolis


#endif  // ARGOLIS_HPP
