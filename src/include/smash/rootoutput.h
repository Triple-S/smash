/*
 *
 *    Copyright (c) 2014-2020
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_ROOTOUTPUT_H_
#define SRC_INCLUDE_ROOTOUTPUT_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include "TFile.h"
#include "TTree.h"

#include "configuration.h"
#include "forwarddeclarations.h"
#include "outputinterface.h"
#include "outputparameters.h"

namespace smash {
class Particles;

/**
 * \ingroup output
 *
 * \brief SMASH output to ROOT file
 * ----------------------------------------------------------------------------
 * SMASH supports ROOT output as an option (see http://root.cern.ch).
 * The ROOT framework needs to be installed when building SMASH, otherwise
 * ROOT support will be disabled.
 *
 * This class produces file smash_run.root, which contains a
 * ROOT TTree. TTree contains information about particles
 * during simulation from all SMASH events.
 * Output is happening in blocks. All particles in a block
 * are at the same time and in the same event. However, it is possible that
 * different blocks are at the same time and from the same event.
 * Particle information is stored in TBranches.
 * For each particle characteristic there is a separate branch.
 * Currently these are t,x,y,z (coordinates), p0,px,py,pz (4-momentum),
 * pdgid - PDG code of particle, that characterizes its sort,
 * charge - electric charge of the particle,
 * ev - number of event particle encountered in,
 * tcounter - number of output block in a given event,
 * npart - number of particles,
 * impact_b - impact parameter, and
 * empty_event - whether there was no interaction between the projectile and
 * the target.
 *
 * Here is an example of ROOT macro to read the ROOT output of SMASH:
 * \code
 * int rootfile_analysis_example() {
 *   // open SMASH output file to be read in
 *   TFile *input_file = TFile::Open("../build/data/0/smash_run.root");
 *   if (input_file->IsOpen()) {
 *     printf("Successfully opened file %s\n", input_file->GetName());
 *   } else {
 *     printf("Error at opening file %s\n", input_file->GetName());
 *   }
 *
 *   // Get a tree from file
 *   TTree *tree = static_cast<TTree*>(input_file->Get("particles"));
 *
 *   // Get number of entries in a tree
 *   Int_t nentries = tree->GetEntries();
 *   printf("Number of entries in a tree is %d\n", nentries);
 *
 *   // This draws p_T distribution at initialization
 *   // tree->Draw("sqrt(px*px + py*py)","tcounter==0");
 *
 *   // This draws 3D momentum space distribution at initialization
 *   tree->Draw("px:py:pz","tcounter==0");
 *
 *   return 0;
 * }
 * \endcode
 * For examples of extracting info from .root file see root.cern.ch
 * To view ROOT file use TBrowser:
 * \code
 * root -l
 * new TBrowser
 * \endcode
 * If option write_collisions is set True, then in addition to particles
 * TTree a collision TTree is created. Information about each collision is
 * written as one leaf: nin, nout - number of incoming and outgoing particles,
 * ev - event number, weight - total weight of the collision (wgt),
 * partial_weight - partial weight of the collision (par_wgt), (t,x,y,z),
 * (p0,px,py,pz) - arrays of dimension nin+nout
 * that contain coordinates and momenta.
 */
class RootOutput : public OutputInterface {
 public:
  /**
   * Construct ROOT output.
   *
   * \param[in] path Output path.
   * \param[in] name Name of the ouput.
   * \param[in] out_par A structure containing parameters of the output.
   */
  RootOutput(const bf::path &path, const std::string &name,
             const OutputParameters &out_par);

  /// Destructor
  ~RootOutput();

  /**
   * update event number and writes intermediate particles to a tree.
   * \param[in] particles Particles to be written to output.
   * \param[in] event_number event number to be used in ROOT output.
   */
  void at_eventstart(const Particles &particles,
                     const int event_number) override;
  /**
   * update event number and impact parameter,
   * and writes intermediate particles to a tree.
   * \param[in] particles Particles to be written to output.
   * \param[in] event_number event number to be used in ROOT output.
   * \param[in] impact_parameter event number to be used in ROOT output. [fm]
   * \param[in] empty_event Whether there was no interaction between the target
   *            and the projectile.
   */
  void at_eventend(const Particles &particles, const int event_number,
                   double impact_parameter, bool empty_event) override;
  /**
   * Writes intermediate particles to a tree defined by treename,
   * if it is allowed (i.e., particles_only_final_ is No).
   * \param[in] particles Particles to be written to output.
   * \param[in] clock Unused, needed since inherited.
   * \param[in] dens_param Unused, needed since inherited.
   */
  void at_intermediate_time(const Particles &particles,
                            const std::unique_ptr<Clock> &clock,
                            const DensityParameters &dens_param) override;
  /**
   * Writes collisions to a tree defined by treename.
   * \param[in] action an Action object containing incoming, outgoing particles
   *            and type of interactions.
   * \param[in] density Unused, needed since inherited.
   */
  void at_interaction(const Action &action, const double density) override;

 private:
  /// Filename of output
  const bf::path filename_;
  /// Filename of output as long as simulation is still running.
  bf::path filename_unfinished_;
  /// Pointer to root output file.
  std::unique_ptr<TFile> root_out_file_;
  /**
   * TTree for particles output.
   *
   * TFile takes ownership of all TTrees.
   * That's why TTree is not a unique pointer.
   */
  TTree *particles_tree_;
  /**
   * TTree for collision output.
   *
   * TFile takes ownership of all TTrees.
   * That's why TTree is not a unique pointer.
   */
  TTree *collisions_tree_;
  /**
   * Writes particles to a tree defined by treename.
   * \param[in] particles Particles or ParticleList to be written to output.
   */
  template <typename T>
  void particles_to_tree(T &particles);
  /**
   * Writes collisions to a tree defined by treename.
   * \param[in] incoming Incoming particles to be written to output.
   * \param[in] outgoing Outgoing particles to be written to output.
   * \param[in] weight Total weight of the collision.
   * \param[in] partial_weight Partial weight of the collision
   */
  void collisions_to_tree(const ParticleList &incoming,
                          const ParticleList &outgoing, const double weight,
                          const double partial_weight);
  /// Number of output in a given event.
  int output_counter_ = 0;
  /// Number of current event.
  int current_event_ = 0;

  /// Maximal buffer size.
  static const int max_buffer_size_ = 10000;

  /** @name Buffer for filling TTree
   * See class documentation for definitions.
   */
  //@{
  /// Property that is written to ROOT output.
  std::array<double, max_buffer_size_> p0, px, py, pz, t, x, y, z,
      formation_time_, xsec_factor_, time_last_coll_;
  std::array<int, max_buffer_size_> pdgcode, charge, coll_per_part_,
      proc_id_origin_, proc_type_origin_, pdg_mother1_, pdg_mother2_;
  int npart, tcounter, ev, nin, nout;
  double wgt, par_wgt, impact_b;
  bool empty_event_;
  //@}

  /// Option to write collisions tree.
  bool write_collisions_;

  /// Option to write particles tree.
  bool write_particles_;

  /// Option to write particles tree for initial conditions
  bool write_initial_conditions_;

  /// Print only final particles in the event, no intermediate output.
  OutputOnlyFinal particles_only_final_;

  /**
   * Root file cannot be read if it was not properly closed and finalized.
   * It can happen that SMASH simulation crashed and root file was not closed.
   * To save results of simulation in such case, "AutoSave" is
   * applied every N events. The autosave_frequency_ sets
   * this N (default N = 1000). Note that "AutoSave" operation is very
   * time-consuming, so the Autosave_Frequency is
   * always a compromise between safety and speed.
   */
  int autosave_frequency_;

  /// Whether extended particle output is on
  const bool part_extended_;
  /// Whether extended collisions output is on
  const bool coll_extended_;
  /// Whether extended ic output is on
  const bool ic_extended_;

  /**
   * Basic initialization routine, creating the TTree objects
   * for particles and collisions.
   */
  void init_trees();
};

}  // namespace smash

#endif  // SRC_INCLUDE_ROOTOUTPUT_H_
