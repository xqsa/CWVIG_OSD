#include "cmaes_interface.h"

extern "C" {
#include "boundary_transformation.h"

void cmaes_boundary_transformation_init(cmaes_boundary_transformation_t*,
    double const* lower_bounds, double const* upper_bounds, unsigned long len_of_bounds);
void cmaes_boundary_transformation_exit(cmaes_boundary_transformation_t*);
void cmaes_boundary_transformation(cmaes_boundary_transformation_t*,
    double const* x, double* y, unsigned long len);
void cmaes_boundary_transformation_inverse(cmaes_boundary_transformation_t* t,
    double const* y, double* x, unsigned long len);
void cmaes_boundary_transformation_shift_into_feasible_preimage(cmaes_boundary_transformation_t* t,
    double const* x, double* x_shifted, unsigned long len);
}

namespace {

void compileOnlyCMAESDependencyCheck()
{
    cmaes_t* evo = nullptr;
    cmaes_boundary_transformation_t* boundaries = nullptr;

    (void)evo;
    (void)boundaries;
    (void)&cmaes_init;
    (void)&cmaes_exit;
    (void)&cmaes_Get;
    (void)&cmaes_NewDouble;
    (void)&cmaes_SamplePopulation;
    (void)&cmaes_ReSampleSingle;
    (void)&cmaes_UpdateDistribution;
    (void)&cmaes_resume_distribution;
    (void)&cmaes_boundary_transformation_init;
    (void)&cmaes_boundary_transformation_exit;
    (void)&cmaes_boundary_transformation;
    (void)&cmaes_boundary_transformation_inverse;
    (void)&cmaes_boundary_transformation_shift_into_feasible_preimage;
}

}  // namespace
