#ifndef OPENSIM_METABOLIC_POWER_PROBE_UMBERGER2010_H_
#define OPENSIM_METABOLIC_POWER_PROBE_UMBERGER2010_H_
/* -------------------------------------------------------------------------- *
 *               OpenSim:  MuscleMetabolicsUmberger2010Probe.h                *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2012 Stanford University and the Authors                *
 * Author(s): Tim Dorn                                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "Probe.h"
#include "Model.h"
#include <OpenSim/Simulation/Model/Muscle.h>


namespace OpenSim { 

// Helper classes defined below.
class MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter;
class MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameterSet;

//=============================================================================
//             MUSCLE METABOLIC POWER PROBE (Umberger, et al., 2010)
//=============================================================================
/**
 * MuscleMetabolicsUmberger2010Probe is a ModelComponent Probe for computing the 
 * net metabolic energy rate of a set of Muscles in the model during a simulation. 
 * 
 * Based on the following papers:
 *
 * <a href="http://www.ncbi.nlm.nih.gov/pubmed/20356877">
 * Umberger, B. R. (2010). Stance and swing phase costs in human walking.
 * J R Soc Interface 7, 1329-40.</a>
 *
 * <a href="http://www.ncbi.nlm.nih.gov/pubmed/12745424">
 * Umberger, B. R., Gerritsen, K. G. and Martin, P. E. (2003). 
 * A model of human muscle energy expenditure. 
 * Comput Methods Biomech Biomed Engin 6, 99-111.</a>
 *
 *
 * <I>Note that the equations below that describe the particular implementation of 
 * MuscleMetabolicsUmberger2010Probe may slightly differ from the equations
 * described in the representative publications above. Note also that we define
 * positive muscle velocity to indicate lengthening (eccentric contraction) and
 * negative muscle velocity to indicate shortening (concentric contraction).</I>
 *
 *
 * Muscle metabolic power (or rate of metabolic energy consumption) is equal to the
 * rate at which heat is liberated plus the rate at which work is done:\n
 * <B>Edot = Bdot + sumOfAllMuscles(Adot + Mdot + Sdot + Wdot).</B>
 *
 *       - Bdot is the basal heat rate (W).
 *       - Adot is the activation heat rate (W).
 *       - Mdot is the maintenance heat rate (W).
 *       - Sdot is the shortening heat rate (W).
 *       - Wdot is the mechanical work rate (W).
 *
 *
 * This probe also uses muscle parameters stored in the MetabolicMuscle object for each muscle.
 * The full set of all MetabolicMuscles (MetabolicMuscleSet) is a property of this probe:
 * 
 * - m = The mass of the muscle (kg).
 * - r = Ratio of slow twitch fibers in the muscle (between 0 and 1).
 *
 *
 *
 * <H2><B> BASAL HEAT RATE (W) </B></H2>
 * If <I>basal_rate_on</I> is set to true, then Bdot is calculated as follows:\n
 * <B>Bdot = basal_coefficient * (m_body^basal_exponent) </B>
 *     - m_body = mass of the entire model
 *     - basal_coefficient and basal_exponent are defined by their respective properties.\n
 * <I>Note that this quantity is muscle independant. Rather it is calculated on a whole body level.</I>
 *
 *
 * <H2><B> ACTIVATION & MAINTENANCE HEAT RATE (W) </B></H2>
 * If <I>activation_maintenance_rate_on</I> is set to true, then Adot+Mdot is calculated as follows:\n
 * <B>m * (Adot+Mdot = [128*(1-r) + 25] * A^0.6 * S)                        </B>,  <I> l_CE <= l_CE_opt </I>\n 
 * <B>m * (Adot+Mdot = 0.4*[128*(1-r) + 25] + 0.6*F_iso*[128*(1-r) + 25])   </B>,  <I> l_CE >  l_CE_opt </I>
 *     - <B>A = u          </B>,    u >  a
 *     - <B>A = (u+a)/2    </B>,    u <= a
 *
 *     - m = The mass of the muscle (kg).
 *     - l_CE = muscle fiber length at the current time.
 *     - l_CE_opt = optimal fiber length of the muscle.
 *     - F_CE_iso = force that would be developed by the contractile element of muscle under isometric conditions with the current activation and fiber length.
 *     - u = muscle excitation at the current time.
 *     - a = muscle activation at the current time.
 *     - S = aerobic/anaerobic scaling factor, defined by the 'aerobic_factor' property (i.e. usually 1.0 for primarily anaerobic activities, 1.5 for primarily aerobic activities).
 *
 *
 * <H2><B> SHORTENING HEAT RATE (W) </B></H2>
 * If <I>shortening_rate_on</I> is set to true, then Sdot is calculated as follows:\n
 * <B>Sdot = m * (-[(alphaS_slow * v_CE_norm * r) + (alphaS_fast * v_CE_norm * (1-r))] * A^2 * S)           </B>,   <I>l_CE <= l_CE_opt   &   v_CE >= 0 (concentric / isometric contraction)</I>\n
 * <B>Sdot = m * (-[(alphaS_slow * v_CE_norm * r) + (alphaS_fast * v_CE_norm * (1-r))] * A^2 * S * F_iso)   </B>,   <I>l_CE >  l_CE_opt   &   v_CE >= 0 (concentric / isometric contraction)</I>\n
 * <B>Sdot = m * (alphaL * v_CE_norm * A * S)           </B>,   <I>l_CE <= l_CE_opt   &   v_CE <  0 (eccentric contraction)</I>\n
 * <B>Sdot = m * (alphaL * v_CE_norm * A * S * F_iso)   </B>,   <I>l_CE >  l_CE_opt   &   v_CE <  0 (eccentric contraction)</I>
 * 
 *     - <B>A = u          </B>,    <I>u >  a </I>
 *     - <B>A = (u+a)/2    </B>,    <I>u <= a </I>
 *
 *     - <B>alphaS_fast = 153 / v_CE_max          </B>
 *     - <B>alphaS_slow = 100 / (v_CE_max / 2.5)  </B>
 *     - <B>alphaL = 0.3 * alphaS_slow </B>
 *
 *     - m = The mass of the muscle (kg).
 *     - l_CE = muscle fiber length at the current time.
 *     - l_CE_opt = optimal fiber length of the muscle.
 *     - F_CE = force developed by the contractile element of muscle at the current time.
 *     - F_CE_iso = force that would be developed by the contractile element of muscle under isometric conditions with the current activation and fiber length.
 *     - v_CE = muscle fiber velocity at the current time.
 *     - v_CE_max = maximum shortening velocity of the muscle.
 *     - v_CE_norm = normalized muscle fiber velocity (defined for this model as v_CE/l_CE_opt).
 *               Note that this is a different metric to the typical normalized_muscle_fiber_velocity of v_CE/v_CE_max.
 *     - S = aerobic/anaerobic scaling factor, defined by the 'aerobic_factor' property (i.e. usually 1.0 for primarily anaerobic activities, 1.5 for primarily aerobic activities).
 *
 *
 * <H2><B> MECHANICAL WORK RATE (W) </B></H2>
 * If <I>mechanical_work_rate_on</I> is set to true, then Wdot is calculated as follows:\n
 * <B>Wdot = -(F_CE * v_CE)           </B>,   <I>v_CE >= 0 (concentric / isometric contraction)</I>\n
 * <B>Wdot = 0                        </B>,   <I>v_CE <  0 (eccentric contraction)</I>
 *     - v_CE = muscle fiber velocity at the current time.
 *     - F_CE = force developed by the contractile element of muscle at the current time.\n
 *
 *
 * Note that if enforce_minimum_heat_rate_per_muscle == true AND 
 * activation_maintenance_rate_on == shortening_rate_on == true, then the total heat
 * rate (AMdot + Sdot) will be capped to a minimum value of 1.0 W/kg (Umberger(2003), page 104).
 *
 *
 *
 *
 * @name MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter
 *
 * MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter is an Object class that 
 * holds the metabolic parameters required to calculate metabolic power for a single muscle.
 *
 * <H2><B> MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter Properties </B></H2>
 *
 * REQUIRED PROPERTIES
 * - <B>specific_tension</B> = The specific tension of the muscle (Pascals (N/m^2)).
 * - <B>density</B> = The density of the muscle (kg/m^3).
 * - <B>ratio_slow_twitch_fibers</B> = Ratio of slow twitch fibers in the muscle (must be between 0 and 1).
 *
 * OPTIONAL PROPERTIES
 * - <B>use_provided_muscle_mass</B> = An optional flag that allows the user to
 *      explicitly specify a muscle mass. If set to true, the <provided_muscle_mass>
 *      property must be specified. The default setting is false, in which case, the
 *      muscle mass is calculated from the following formula:
 *          m = (Fmax/specific_tension)*density*Lm_opt, where 
 *              specific_tension and density are properties defined above
 *                  (note that their default values are set based on mammalian muscle,
 *                  0.25e6 N/m^2 and 1059.7 kg/m^3, respectively);
 *              Fmax and Lm_opt are the maximum isometric force and optimal 
 *                  fiber length, respectively, of the muscle.
 *
 * - <B>provided_muscle_mass</B> = The user specified muscle mass (kg).
 *
 *
 * @author Tim Dorn
 */

class OSIMSIMULATION_API MuscleMetabolicsUmberger2010Probe : public Probe {
OpenSim_DECLARE_CONCRETE_OBJECT(MuscleMetabolicsUmberger2010Probe, Probe);
public:
//==============================================================================
// PROPERTIES
//==============================================================================
    /** @name Property declarations
    These are the serializable properties associated with this class. **/
    /**@{**/
    /** Enabled by default. **/
    OpenSim_DECLARE_PROPERTY(activation_maintenance_rate_on, 
        bool,
        "Specify whether activation & maintenance heat rate is to be calculated (true/false).");

    /** Enabled by default. **/
    OpenSim_DECLARE_PROPERTY(shortening_rate_on, 
        bool,
        "Specify whether shortening heat rate is to be calculated (true/false).");

    /** Enabled by default. **/
    OpenSim_DECLARE_PROPERTY(basal_rate_on, 
        bool,
        "Specify whether basal heat rate is to be calculated (true/false).");

    /** Enabled by default. **/
    OpenSim_DECLARE_PROPERTY(mechanical_work_rate_on, 
        bool,
        "Specify whether mechanical work rate is to be calculated (true/false).");

    /** Enabled by default. **/
    OpenSim_DECLARE_PROPERTY(enforce_minimum_heat_rate_per_muscle, 
        bool,
        "Specify whether the total heat rate for a muscle will be clamped to a "
        "minimum value of 1.0 W/kg (true/false).");

    /** Default value = 1.5. **/
    OpenSim_DECLARE_PROPERTY(aerobic_factor, 
        double,
        "Aerobic scale factor (S=1.0 for primarily anaerobic conditions and S=1.5 "
        "for primarily aerobic conditions. See Umberger et al., (2003).");

    /** Default value = 1.2. **/
    OpenSim_DECLARE_PROPERTY(basal_coefficient, 
        double,
        "Basal metabolic coefficient.");

    /** Default value = 1.0. **/
    OpenSim_DECLARE_PROPERTY(basal_exponent, 
        double,
        "Basal metabolic exponent.");

    OpenSim_DECLARE_UNNAMED_PROPERTY(
        MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameterSet,
        "A set containing, for each muscle, the parameters "
        "required to calculate metabolic energy expenditure. If multiple "
        "muscles are contained in the set, then the probe will sum the "
        "metabolic powers from all muscles.");

    /**@}**/

//=============================================================================
// PUBLIC METHODS
//=============================================================================
    typedef std::map
       <std::string, 
        MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter*> 
        MuscleMap;

    //--------------------------------------------------------------------------
    // Constructor(s) and Setup
    //--------------------------------------------------------------------------
    /** Default constructor */
    MuscleMetabolicsUmberger2010Probe();

    /** Convenience constructor */
    MuscleMetabolicsUmberger2010Probe(
        const bool activation_maintenance_rate_on, 
        const bool shortening_rate_on, 
        const bool basal_rate_on, 
        const bool work_rate_on);


    //-----------------------------------------------------------------------------
    // Computation
    //-----------------------------------------------------------------------------
    /** Compute muscle metabolic power. */
    virtual SimTK::Vector computeProbeInputs(const SimTK::State& state) const OVERRIDE_11;

    /** Returns the number of probe inputs in the vector returned by computeProbeInputs(). */
    int getNumProbeInputs() const OVERRIDE_11;

    /** Returns the column labels of the probe values for reporting. 
        Currently uses the Probe name as the column label, so be sure
        to name your probe appropiately!  */
    virtual OpenSim::Array<std::string> getProbeOutputLabels() const OVERRIDE_11;


    //-----------------------------------------------------------------------------
    /** @name     MuscleMetabolicsUmberger2010Probe Interface
    These accessor methods are to be used when setting up a new muscle 
    metabolic analysis from the API. The basic operation is as follows:
    @code
    MuscleMetabolicsUmberger2010Probe* myProbe new MuscleMetabolicsUmberger2010Probe(...); 
    model.addProbe(myProbe);
    myProbe->addMuscle("muscleName1", ... );
    myProbe->addMuscle("muscleName2", ... );
    myProbe->addMuscle("muscleName3", ... );
    myProbe->removeMuscle("muscleName3");
    myProbe->setOperation("integrate")           // See OpenSim::Probe for other operations
    @endcode
    @note It is important to first add the metabolic probe to the model before
    calling any other methods that may modify its properties. This is because 
    some methods (e.g. addMuscle() or useCalculatedMass) may require information
    about the muscles to sucsessfully execute, and this information can only be
    obtained if the metabolic probe is already 'connected' to the model.
    */
    /** Get the number of muscles being analysed in the metabolic analysis. */
    const int getNumMetabolicMuscles() const;  

    /** Add a muscle and its parameters so that it can be included in the metabolic analysis. */
    void addMuscle(const std::string& muscleName, 
        double ratio_slow_twitch_fibers);

    /** Add a muscle and its parameters so that it can be included in the metabolic analysis. */
    void addMuscle(const std::string& muscleName, 
        double ratio_slow_twitch_fibers, 
        double muscle_mass);

    /** Remove a muscle from the metabolic analysis. */
    void removeMuscle(const std::string& muscleName);

    /** Set an existing muscle to use a provided muscle mass. */
    void useProvidedMass(const std::string& muscleName, double providedMass);
    
    /** Set an existing muscle to calculate its own mass. */
    void useCalculatedMass(const std::string& muscleName);

    /** Get whether the muscle mass is being explicitly provided.
        True means that it is using the property <provided_muscle_mass>
        False means that the muscle mass is being calculated from muscle properties. */
    bool isUsingProvidedMass(const std::string& muscleName);

    /** Get the muscle mass used in the metabolic analysis. The value
        returned will depend on if the muscle mass is explicitly provided
        (i.e. isUsingProvidedMass = true), or if it is being automatically
        calculated from muscle data already present in the model
        (i.e. isUsingProvidedMass = true). */
    const double getMuscleMass(const std::string& muscleName) const;

    /** Get the ratio of slow twitch fibers for an existing muscle. */
    const double getRatioSlowTwitchFibers(const std::string& muscleName) const;

    /** Set the ratio of slow twitch fibers for an existing muscle. */
    void setRatioSlowTwitchFibers(const std::string& muscleName, const double& ratio);



//==============================================================================
// PRIVATE
//==============================================================================
private:
    //--------------------------------------------------------------------------
    // Data
    //--------------------------------------------------------------------------
    MuscleMap _muscleMap;

    //--------------------------------------------------------------------------
    // ModelComponent Interface
    //--------------------------------------------------------------------------
    void connectToModel(Model& aModel) OVERRIDE_11;
    void connectIndividualMetabolicMuscle
       (Model& aModel, 
        MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter& mm);

    void setNull();
    void constructProperties();


    //--------------------------------------------------------------------------
    // MetabolicMuscleParameter Private Interface
    //--------------------------------------------------------------------------
    // Get const MetabolicMuscleParameter from the MuscleMap using a string accessor.
    const MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter* 
        getMetabolicParameters(const std::string& muscleName) const;

    // Get writable MetabolicMuscleParameter from the MuscleMap using a string accessor.
    MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter* 
        updMetabolicParameters(const std::string& muscleName);

public:


//=============================================================================
};	// END of class MuscleMetabolicsUmberger2010Probe
//=============================================================================

//==============================================================================
//          MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter
//==============================================================================
class OSIMSIMULATION_API 
    MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter 
    : public Object  
{
    OpenSim_DECLARE_CONCRETE_OBJECT(
        MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter, Object);
public:
//==============================================================================
// PROPERTIES
//==============================================================================
    /** @name Property declarations
    These are the serializable properties associated with this class. **/
    /**@{**/
    OpenSim_DECLARE_PROPERTY(specific_tension, double,
        "The specific tension of the muscle (Pascals (N/m^2)).");

    OpenSim_DECLARE_PROPERTY(density, double,
        "The density of the muscle (kg/m^3).");

    OpenSim_DECLARE_PROPERTY(ratio_slow_twitch_fibers, double,
        "Ratio of slow twitch fibers in the muscle (must be between 0 and 1).");

    OpenSim_DECLARE_PROPERTY(use_provided_muscle_mass, bool,
        "An optional flag that allows the user to explicitly specify a muscle mass. "
        "If set to true, the <provided_muscle_mass> property must be specified.");

    OpenSim_DECLARE_PROPERTY(provided_muscle_mass, double,
        "The user specified muscle mass (kg).");
    /**@}**/

    //=============================================================================
    // METHODS
    //=============================================================================
    //--------------------------------------------------------------------------
    // Constructor(s)
    //--------------------------------------------------------------------------
    MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter(); 

    MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter(
        const std::string& muscleName,
        double ratio_slow_twitch_fibers, 
        double muscle_mass = SimTK::NaN);

    //--------------------------------------------------------------------------
    // Muscle mass
    //--------------------------------------------------------------------------
    const double& getMuscleMass() const      { return _muscMass; }
    void setMuscleMass();

    //--------------------------------------------------------------------------
    // Internal muscle pointer
    //--------------------------------------------------------------------------
    const Muscle* getMuscle() const         { return _musc; }
    void setMuscle(Muscle* m)               { _musc = m; }


    //--------------------------------------------------------------------------
    // Object interface
    //--------------------------------------------------------------------------
private:
    void setNull();
    void constructProperties();

    //=============================================================================
    // DATA
    //=============================================================================
    // These private member variables are set by the probe that owns this
    // MetabolicMuscleParameter
    SimTK::ReferencePtr<Muscle> _musc;  // Internal pointer to the muscle that corresponds
                            // to these parameters.
    double _muscMass;       // The mass of the muscle (depends on if
                            // <use_provided_muscle_mass> is true or false.

//=============================================================================
};	// END of class MetabolicMuscleParameter
//=============================================================================



//==============================================================================
//                          MetabolicMuscleParameterSet
//==============================================================================
/**
 * MetabolicMuscleParameterSet is an internal container class containing the set 
 * of MetabolicMuscleParameters for each muscle that is probed.
 */
class OSIMSIMULATION_API 
    MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameterSet 
    : public Set<MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter>
{
    OpenSim_DECLARE_CONCRETE_OBJECT(
        MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameterSet, 
        Set<MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameter>);

public:
    MuscleMetabolicsUmberger2010Probe_MetabolicMuscleParameterSet()  
    {  }   

//=============================================================================
};	// END of class MetabolicMuscleParameterSet
//=============================================================================


}; //namespace
//=============================================================================
//=============================================================================


#endif // #ifndef OPENSIM_METABOLIC_POWER_PROBE_UMBERGER2010_H_