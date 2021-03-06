/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2013 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

// NOTE: This is an implementation header file and is only meant to be included
//    from implementation files. It therefore doesn't have an include guard.

// mapnik
#include <mapnik/params.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/boolean.hpp>
#include <mapnik/util/conversions.hpp>
// boost
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp> // keep gcc happy
#include <boost/variant/variant.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
// stl
#include <string>

namespace mapnik { namespace detail {


template <typename T>
struct extract_value
{
    static inline boost::optional<T> do_extract_from_string(std::string const& source)
    {
        std::string err_msg = (boost::format("No conversion from std::string to %s") % typeid(T).name()).str();
        throw std::runtime_error(err_msg);
    }
};

template <>
struct extract_value<mapnik::boolean>
{
    static inline boost::optional<mapnik::boolean> do_extract_from_string(std::string const& source)
    {
        bool result;
        if (mapnik::util::string2bool(source, result))
            return boost::optional<mapnik::boolean>(result);
        return boost::optional<mapnik::boolean>();
    }
};

template <>
struct extract_value<int>
{
    static inline boost::optional<int> do_extract_from_string(std::string const& source)
    {
        mapnik::value_integer result;
        if (mapnik::util::string2int(source, result))
            return boost::optional<int>(result);
        return boost::optional<int>();
    }
};

#ifdef BIGINT

template <>
struct extract_value<mapnik::value_integer>
{
    static inline boost::optional<mapnik::value_integer> do_extract_from_string(std::string const& source)
    {
        mapnik::value_integer result;
        if (mapnik::util::string2int(source, result))
            return boost::optional<mapnik::value_integer>(result);
        return boost::optional<mapnik::value_integer>();
    }
};

#endif

template <>
struct extract_value<mapnik::value_double>
{
    static inline boost::optional<mapnik::value_double> do_extract_from_string(std::string const& source)
    {
        mapnik::value_double result;
        if (mapnik::util::string2double(source, result))
            return boost::optional<double>(result);
        return boost::optional<double>();
    }
};

template <>
struct extract_value<mapnik::value_null>
{
    static inline boost::optional<mapnik::value_null> do_extract_from_string(std::string const& source)
    {
        return boost::optional<mapnik::value_null>(); // FIXME
    }
};


template <>
struct extract_value<std::string>
{
    static inline boost::optional<std::string> do_extract_from_string(std::string const& source)
    {
        return boost::optional<std::string>(source);
    }
};



template <typename T>
boost::optional<T> param_cast(std::string const& source)
{
    return extract_value<T>::do_extract_from_string(source);
}

} // end namespace detail

template <typename T>
struct value_extractor_visitor : public boost::static_visitor<>
{

    value_extractor_visitor(boost::optional<T> & var)
        :var_(var) {}

    void operator() (std::string const& val) const
    {
        var_ = detail::param_cast<T>(val);

    }

    template <typename T1>
    void operator () (T1 val) const
    {
        try
        {
            var_ = boost::lexical_cast<T>(val);
        }
        catch (boost::bad_lexical_cast const& )
        {
            std::string err_msg = (boost::format("Failed converting from %s to %s")
                                   % typeid(T1).name()
                                   % typeid(T).name()).str();
            throw std::runtime_error(err_msg);
        }
    }


    boost::optional<T> & var_;
};

namespace params_detail {

template <typename T>
struct converter
{
    typedef T value_type;
    typedef boost::optional<value_type> return_type;
    static return_type extract(parameters const& params,
                               std::string const& name,
                               boost::optional<T> const& default_opt_value)
    {
        boost::optional<T> result(default_opt_value);
        parameters::const_iterator itr = params.find(name);
        if (itr != params.end())
        {
            boost::apply_visitor(value_extractor_visitor<T>(result),itr->second);
        }
        return result;
    }
};

} // end namespace params_detail


template <typename T>
boost::optional<T> parameters::get(std::string const& key) const
{
    return params_detail::converter<T>::extract(*this,key, boost::none);
}

template <typename T>
boost::optional<T> parameters::get(std::string const& key, T const& default_opt_value) const
{
    return params_detail::converter<T>::extract(*this,key,boost::optional<T>(default_opt_value));
}


}
