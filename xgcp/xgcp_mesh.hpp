#pragma once
#include "xgcp_types.hpp"
#include "xgcp_input.hpp"
#include <pumipic_mesh.hpp>

/*
  XGCp Mesh encapsulates the partitioning of the mesh, torodial planes, and groups

  Partition Description:
    Mesh is partitioned into picparts based on pumipic::Mesh rules

    Torodial direction is partitioned based on planes with each process
      maintaining a major and minor plane. Every plane will be copied on two
      processes and will be major and minor on exactly one. The process
      with the plane as its major plane 'owns' the plane and will be
      responsible for solving and updating the plane values.

    Groups are copies of the same picpart/plane. Each group has a group leader
      and group members. Fields are accumulated to the group leader and scattered
      back to the members.
 */

namespace xgcp {
  class Mesh {
  public:
    Mesh(xgcp::Input&);
    ~Mesh();
    //Delete default compilers
    Mesh() = delete;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    //Directly access underlying mesh structures
    o::Mesh* omegaMesh() {return picparts->mesh();}
    p::Mesh* pumipicMesh() {return picparts;}

    /********Common mesh count information********/
    //Returns the dimension of the mesh
    int dim() const {return picparts->dim();}
    //Returns the number of entities of the picpart
    Omega_h::LO nents(int dim) const {return picparts->nents(dim);}
    //Returns the number of elements of the picpart
    Omega_h::LO nelems() const {return picparts->nelems();}

    /********Comm Rank/Size/Comm functions********/
    int worldRank() {return omegaMesh()->library()->world()->rank();}
    int worldSize() {return omegaMesh()->library()->world()->size();}
    MPI_Comm worldComm() {return omegaMesh()->library()->world()->get_impl();}

    int meshRank() {return mesh_comm->rank();}
    int meshSize() {return mesh_comm->size();}
    MPI_Comm meshComm() {return mesh_comm->get_impl();}

    int groupRank() {return group_comm->rank();}
    int groupSize() {return group_comm->size();}
    MPI_Comm groupComm() {return group_comm->get_impl();}
    bool isGroupLeader() {return group_comm->rank() == 0;}

    int torodialRank() {return torodial_comm->rank();}
    int torodialSize() {return torodial_comm->size();}
    MPI_Comm torodialComm() {return torodial_comm->get_impl();}

    int planeID() {return torodialRank();}
    fp_t getMajorPlaneAngle() {return major_phi;}
    fp_t getMinorPlaneAngle() {return minor_phi;}

    enum PartitionLevel {
      GROUP,
      TORODIAL,
      MESH
    };
    /*
      Accumulates field contributions across different partitioning levels
       * dim - the dimension of the mesh
       * field - the field being reduced
       * start_level(optional) - The initial partitioning level (defaults to GROUP)
       * end_level(optional) - The final partitioning level (defaults to MESH)

       Field contributions are accumulated across the mesh entities of dimension `dim`
       from `start_level` to `end_level` inclusively. The accumulation order is
       GROUP > TORODIAL > MESH. By default all levels will be accumulated.

       GROUP gather is done by summing contributions to each group leader.
       TORODIAL gather is done by sending contriutions of each group leader's minor plane
         to the neighboring plane's major plane
       MESH gather is done using pumipic::Mesh's reduceCommArray for each group
         leader's major plane

       Note: end_level must be greater than start_level
     */
    template <class T>
    void gatherField(int dim, Omega_h::Write<T> field,
                     PartitionLevel start_level = GROUP, PartitionLevel end_level = MESH);

    /*
      Scatters field values across different partitioning levels
       * dim - the dimension of the mesh
       * field - the field being reduced
       * start_level(optional) - The initial partitioning level (defaults to GROUP)
       * end_level(optional) - The final partitioning level (defaults to MESH)

       Field contributions are scattered across the mesh entities of dimension `dim`
       from `start_level` to `end_level` inclusively. The accumulation order is
       MESH > TORODIAL > GROUP. By default all levels will be accumulated.

       MESH scatter is performed using pumipic::Mesh's reduceCommArray with max
         operation for each group leader's major plane
       TORODIAL scatter is done by sending values of each group leader's major plane
         to the neighboring plane's minor plane
       GROUP scatter is done by scattering field values from the group leader to each group member

       Note: end_level must be greater than start_level
     */
    template <class T>
    void scatterField(int dim, Omega_h::Write<T> field,
                      PartitionLevel start_level = MESH, PartitionLevel end_level = GROUP);

  private:
    //Mesh structure for the picparts
    p::Mesh* picparts;
    o::Mesh* full_mesh;

    //Communicators for each partitioning direction
    o::CommPtr mesh_comm, torodial_comm, group_comm;

    //Torodial angle of the plane that this process is not leader of
    fp_t minor_phi;
    //Torodial angle of the plane that this process is leader of
    fp_t major_phi;

    //Forward and backward ion projection mapping for each ring point to 3 mesh vertices
    Omega_h::LOs forward_ion_gyro_map, backward_ion_gyro_map;

    //Forward and backward electron projection mapping for each vertex to 3 mesh vertices
    Omega_h::LOs forward_electron_gyro_map, backward_electron_gyro_map;

  };
}