#include "pumipic_adjacency.hpp"
#include "GitrmMesh.hpp"
#include "GitrmParticles.hpp"

#include <Kokkos_Core.hpp>
#include <chrono>

namespace o = Omega_h;
namespace p = pumipic;



void printTiming(const char* name, double t) {
  fprintf(stderr, "kokkos %s (seconds) %f\n", name, t);
}

void printTimerResolution() {
  Kokkos::Timer timer;
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  fprintf(stderr, "kokkos timer reports 1ms as %f seconds\n", timer.seconds());
}


int main(int argc, char** argv) {
  Kokkos::initialize(argc,argv);
  printf("particle_structs floating point value size (bits): %zu\n", sizeof(fp_t));
  printf("omega_h floating point value size (bits): %zu\n", sizeof(Omega_h::Real));
  printf("Kokkos execution space memory %s name %s\n",
      typeid (Kokkos::DefaultExecutionSpace::memory_space).name(),
      typeid (Kokkos::DefaultExecutionSpace).name());
  printf("Kokkos host execution space %s name %s\n",
      typeid (Kokkos::DefaultHostExecutionSpace::memory_space).name(),
      typeid (Kokkos::DefaultHostExecutionSpace).name());
  printTimerResolution();

  if(argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <mesh>\n";
    exit(1);
  }

  auto lib = Omega_h::Library(&argc, &argv);
  const auto world = lib.world();
  auto mesh = Omega_h::gmsh::read(argv[1], world);
  const auto r2v = mesh.ask_elem_verts();
  const auto coords = mesh.coords();

  Omega_h::Int ne = mesh.nelems();
  fprintf(stderr, "number of elements %d \n", ne);



  //associate one floating point value with each mesh vertex and set them to 0
//  mesh.add_tag(o::VERT, "avg_density", 1, o::Reals(mesh.nverts(), 0));

  //GitrmMesh &gm = GitrmMesh::getInstance(&mesh);
  GitrmMesh gm(mesh);
  Kokkos::Timer timer;
  // Add bdry faces to elements within 1mm

  //preProcessDistToBdry(mesh, bdryFacesW, numBdryFaceIds, bdryFaceIds, bdryFlags);
  gm.preProcessDistToBdry();
  gm.printBdryFaceIds(false, 25); //numBdryFaceIds, mesh);  
  gm.printBdryFacesCSR(false, 25);


  GitrmParticles gp(mesh); // (const char* param_file);
  gitrm_findDistanceToBdry(gp.scs, r2v, coords, gm.bdryFaces, gm.bdryFaceInds, 
      SIZE_PER_FACE, FSKIP, ne);

  fprintf(stderr, "time (seconds) %f\n", timer.seconds());
  timer.reset();

  //p::test_find_closest_point_on_triangle();
  p::test_find_distance_to_bdry();

  Omega_h::vtk::write_parallel("pisces", &mesh, mesh.dim());


  fprintf(stderr, "done\n");
  return 0;
}


