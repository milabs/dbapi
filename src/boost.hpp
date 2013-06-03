#ifndef __BOOST_HPP__
#define __BOOST_HPP__

#include <boost/foreach.hpp>
#include <boost/current_function.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/system/system_error.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef DEBUG
# define NOT_COMPLETED()			\
	fprintf(stderr, "NOT_COMPLETED: %s\n", BOOST_CURRENT_FUNCTION)
# define NOT_IMPLEMENTED()			\
	fprintf(stderr, "NOT_IMPLEMENTED: %s\n", BOOST_CURRENT_FUNCTION)
#else
# define NOT_COMPLETED()
# define NOT_IMPLEMENTED()
#endif

namespace fs = boost::filesystem;
namespace al = boost::algorithm;
namespace pt = boost::property_tree;

#endif /* __BOOST_HPP__ */
