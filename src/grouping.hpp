#pragma once
#include "element_table.hpp"
#include "pde.hpp"
#include "tensors.hpp"

using element_group = std::map<int, std::pair<int, int>>;

// convenience functions when working with element groups
int num_elements_in_group(element_group const &g);
int max_connected_in_group(element_group const &g);

std::pair<int, int> columns_in_group(element_group const &g);
std::pair<int, int> rows_in_group(element_group const &g);

// FIXME we should eventually put this in the pde class?
auto const element_segment_size = [](auto const &pde) {
  int const degree = pde.get_dimensions()[0].get_degree();
  return static_cast<int>(std::pow(degree, pde.num_dims));
};

// workspace for the primary computation in time advance. along with
// the coefficient matrices, we need this space resident on whatever
// accelerator we are using
template<typename P>
class rank_workspace
{
public:
  rank_workspace(PDE<P> const &pde, std::vector<element_group> const &groups);
  fk::vector<P> const &get_unit_vector() const;
  // input, output, workspace for batched gemm/reduction
  fk::vector<P> batch_input;
  fk::vector<P> reduction_space;
  fk::vector<P> batch_intermediate;
  fk::vector<P> batch_output;
  double size_MB() const
  {
    int64_t num_elems = batch_input.size() + reduction_space.size() +
                        batch_intermediate.size() + batch_output.size() +
                        unit_vector_.size();
    double const bytes     = static_cast<double>(num_elems) * sizeof(P);
    double const megabytes = bytes * 1e-6;
    return megabytes;
  };

private:
  fk::vector<P> unit_vector_;
};

// larger, host-side memory space holding the entire input/output vectors.
// FIXME when we split the problem with MPI, we can restrict this to
// only the portions of x and y needed for a given rank's
// assigned element groups.
template<typename P>
class host_workspace
{
public:
  host_workspace(PDE<P> const &pde, element_table const &table);
  // working vectors for time advance (e.g. intermediate RK result vects,
  // source vector space)
  fk::vector<P> scaled_source;
  fk::vector<P> x_orig;
  fk::vector<P> x;
  fk::vector<P> fx;
  fk::vector<P> result_1;
  fk::vector<P> result_2;
  fk::vector<P> result_3;

  double size_MB() const
  {
    int64_t num_elems = scaled_source.size() + x_orig.size() + fx.size() +
                        x.size() + result_1.size() + result_2.size() +
                        result_3.size();
    double const bytes     = static_cast<double>(num_elems) * sizeof(P);
    double const megabytes = bytes * 1e-6;
    return megabytes;
  };
};

// functions to assign groups
template<typename P>
int get_num_groups(element_table const &table, PDE<P> const &pde,
                   int const num_ranks = 1, int const rank_size_MB = 1000);

std::vector<element_group>
assign_elements(element_table const &table, int const num_groups);

// data management functions
template<typename P>
void copy_group_inputs(PDE<P> const &pde, rank_workspace<P> &rank_space,
                       host_workspace<P> const &host_space,
                       element_group const &group);

template<typename P>
void copy_group_outputs(PDE<P> const &pde, rank_workspace<P> &rank_space,
                        host_workspace<P> const &host_space,
                        element_group const &group);

// reduce an element group's results after batched gemm
template<typename P>
void reduce_group(PDE<P> const &pde, rank_workspace<P> &rank_space,
                  element_group const &group);

extern template int get_num_groups(element_table const &table,
                                   PDE<float> const &pde, int const num_ranks,
                                   int const rank_size_MB);
extern template int get_num_groups(element_table const &table,
                                   PDE<double> const &pde, int const num_ranks,
                                   int const rank_size_MB);

extern template void copy_group_inputs(PDE<float> const &pde,
                                       rank_workspace<float> &rank_space,
                                       host_workspace<float> const &host_space,
                                       element_group const &group);

extern template void copy_group_inputs(PDE<double> const &pde,
                                       rank_workspace<double> &rank_space,
                                       host_workspace<double> const &host_space,
                                       element_group const &group);

extern template void copy_group_outputs(PDE<float> const &pde,
                                        rank_workspace<float> &rank_space,
                                        host_workspace<float> const &host_space,
                                        element_group const &group);

extern template void
copy_group_outputs(PDE<double> const &pde, rank_workspace<double> &rank_space,
                   host_workspace<double> const &host_space,
                   element_group const &group);

extern template void reduce_group(PDE<float> const &pde,
                                  rank_workspace<float> &rank_space,
                                  element_group const &group);

extern template void reduce_group(PDE<double> const &pde,
                                  rank_workspace<double> &rank_space,
                                  element_group const &group);
