/*
 * rosam_boost_adaptor.hpp
 *
 *  Created on: Jul 4, 2017
 *      Author: ivizzo
 */

#ifndef ROSAM_BOOST_ADAPTOR_HPP_
#define ROSAM_BOOST_ADAPTOR_HPP_

/**
 * @brief This adaptor is intended to teach
 * the boost.polygon library how to deal with
 * the rosam polygons.
 *
 * Including this module in your program you will
 * be able to use the methods of the boost.polygon
 * library with the rosam polygons.
 *
 * As a standard adaptor we need to include the template
 * library and the  "user" library where the polygon are
 * defined as well
 */
#include <boost/polygon/polygon.hpp>
#include <gtsam/geometry/Pose2.h>

/**
 * We define the rosam_polygon_t data type
 * as a vector of gtsam::Pose2. Right now
 * is implemented in this way on the code.
 * If some day this change, we should change
 * only this definition
 */
#include <vector>
typedef std::vector<gtsam::Pose2> rosam_polygon_t;
//typedef std::vector<gtsam::Point2> rosam_polygon_t;

/**
 * We use a vector of Pose2, because we are interested
 * in keep the heading information. But for the geometry
 * purposes we only need the Translation information
 * of the robot. We don't discard the heading though.
 */
//typedef gtsam::Pose2::Translation rosam_point_t;
/// \todo re-define if we really want to do this in this way
/// of we want to specify the gtsam::Point2 as the point concept.
typedef gtsam::Pose2 rosam_point_t;

namespace gtl = boost::polygon;
using namespace boost::polygon::operators;

/**
 * PointType template specialization
 * <boost/polygon/point_traits.hpp>
 */
namespace boost
{
  namespace polygon
  {
    template<>
      struct geometry_concept<rosam_point_t>
      {
        typedef point_concept type;
      };

    template<>
      struct point_traits<rosam_point_t>
      {
        typedef double coordinate_type;

        static inline coordinate_type
        get (const rosam_point_t &point, orientation_2d orient)
        {
          if (orient == HORIZONTAL)
            return point.x ();
          else
            return point.y ();
        }
      };

// ToDo : check if the set method is working
    template<>
      struct point_mutable_traits<rosam_point_t>
      {
        typedef double coordinate_type;

        static inline void
        set (rosam_point_t &point, orientation_2d orient, coordinate_type value)
        {
          if (orient == HORIZONTAL)
            {
              point = rosam_point_t (value, point.y (), point.theta ());
            }
          else
            {
              point = rosam_point_t (point.x (), value, point.theta ());
            }
        }
        static inline rosam_point_t
        construct (coordinate_type x, coordinate_type y)
        {
          // we put 0 because if we will be
          // dealing with the polygons inside
          // the boost.polygon library, we really
          // don't care about the heading. And if
          // we are using this constructor, is because
          // we are building a polygon to use
          // with the library.
          return rosam_point_t (x, y, 0.0);
        }
      };
  } // polygon
} // boost

/**
 * Now we need to specialize our polygon concept
 * mapping in boost polygon:
 * <boost/polygon/polygon_traits.hpp>
 */
namespace boost
{
  namespace polygon
  {
//first register rosam_polygon_t as a polygon_concept type
    template<>
      struct geometry_concept<rosam_polygon_t>
      {
        typedef polygon_concept type;
      };

    template<>
      struct polygon_traits<rosam_polygon_t>
      {
        typedef double coordinate_type;
        typedef rosam_polygon_t::const_iterator iterator_type;
        typedef rosam_point_t point_type;

        // Get the begin iterator
        static inline iterator_type
        begin_points (const rosam_polygon_t &t)
        {
          return t.begin ();
        }

        // Get the end iterator
        static inline iterator_type
        end_points (const rosam_polygon_t &t)
        {
          return t.end ();
        }

        // Get the number of sides of the polygon
        static inline std::size_t
        size (const rosam_polygon_t &t)
        {
          return t.size ();
        }

        // Get the winding direction of the polygon
        static inline winding_direction
        winding (const rosam_polygon_t &t)
        {
          return unknown_winding;
        }
      };

    template<>
      struct polygon_mutable_traits<rosam_polygon_t>
      {
        //expects stl style iterators
        template<typename iT>
          static inline rosam_polygon_t &
          set_points (rosam_polygon_t &t, iT input_begin, iT input_end)
          {
            t.clear ();
            t.insert (t.end (), input_begin, input_end);
            return t;
          }
      };
  }
}

/**
 * We could use the std::vector or std::list
 * specialization that it's already written.
 * But because there is an issue with the get
 * method(basically it can't construct the
 * gstam::Pose2 based polygon) we need to
 * write our own specialization.
 */

//OK, finally we get to declare our own polygon set type
typedef std::deque<rosam_polygon_t> rosam_polygon_set_t;

//deque isn't automatically a polygon set in the library
//because it is a standard container there is a shortcut
//for mapping it to polygon set concept, but I'll do it
//the long way that you would use in the general case.
namespace boost
{
  namespace polygon
  {
//first we register rosam_polygon_set_t as a polygon set
    template<>
      struct geometry_concept<rosam_polygon_set_t>
      {
        typedef polygon_set_concept type;
      };

//next we map to the concept through traits
    template<>
      struct polygon_set_traits<rosam_polygon_set_t>
      {
        typedef double coordinate_type;
        typedef rosam_polygon_set_t::const_iterator iterator_type;
        typedef rosam_polygon_set_t operator_arg_type;

        static inline iterator_type
        begin (const rosam_polygon_set_t &polygon_set)
        {
          return polygon_set.begin ();
        }

        static inline iterator_type
        end (const rosam_polygon_set_t &polygon_set)
        {
          return polygon_set.end ();
        }

        //don't worry about these, just return false from them
        static inline bool
        clean (const rosam_polygon_set_t &polygon_set)
        {
          return false;
        }
        static inline bool
        sorted (const rosam_polygon_set_t &polygon_set)
        {
          return false;
        }
      };

    template<>
      struct polygon_set_mutable_traits<rosam_polygon_set_t>
      {
      template <typename input_iterator_type>
          static inline void set(rosam_polygon_set_t& polygon_set, input_iterator_type input_begin, input_iterator_type input_end) {
            polygon_set.clear();
            polygon_set_data<double> psd;
            psd.insert(input_begin, input_end);

            // now we have our data copied into
            // the psd polygon set. We need to
            // copy that data into the user defined
            // polygon.

//            psd.get(polygon_set);
        }
      };
  }
}


#endif /* ROSAM_BOOST_ADAPTOR_HPP_ */
