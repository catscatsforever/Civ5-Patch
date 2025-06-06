// Defines.h
#pragma once

#ifndef DEFINES_LIST
// List of defines that are supposed to be for new additions and can be easily toggled on and off (plus they help with merging)
#define DEFINES_LIST

// New mathematical constants
// Delnar: these could be calculated at compile-time instead of needing defines, but I don't trust the VC compiler
#define M_E			2.71828182845904523536
#define fM_E		2.718281828f		//!< e (float)
#define M_SQRT2		1.41421356237309504880
#define fM_SQRT2	1.414213562f		//!< sqrt(2) (float)
#define M_SQRT3		1.73205080756887729353
#define fM_SQRT3	1.732050808f		//!< sqrt(3) (float)
#define M_LN2		0.693147180559945309417
#define fM_LN2		0.6931471806f		//!< ln(2) (float)
#define M_GLDNRT	1.61803398874989484820
#define fM_GLDNRT	1.618033989f		//!< (1 + sqrt(5))/2 (float), aka The Golden Ratio

// Technical Improvements
/// New GUID for NQMod
#define MYMOD_GUID
/*
/// Enables Minidump Generation (originally for Civ4 by terkhen, ported to Civ5 by ls612)
#define NQM_MINIDUMPS

/// Can cache doubles from XML (Delnar: DatabaseUtility actually supports double-type, don't know why Firaxis didn't bother putting this in for good measure)
#define NQM_CACHE_DOUBLE
/// Enables const for functions, variables, and parameters that both allow it and are intended to be const
#define AUI_CONSTIFY
/// Replaces instances of vector loops using indeces with ones that use iterators
#define AUI_ITERATORIZE
/// Removes unused functions that simply increase file size of the DLL without providing any benefit
#define NQM_PRUNING
/// Fixes some sources for level 4 warnings
#define AUI_WARNING_FIXES
/// Changes the scopes of certain functions to fall in line with other functions of the same type (eg. CvUnit::CanFallBackFromMelee() is public instead of protected)
#define AUI_SCOPE_FIXES
/// Replaces all instances of iterators with postfix incrementors to have prefix incrementors, increasing performance
#define AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
/// Fast comparison functions (to be used for built-in types like int, float, double, etc.)
#define NQM_FAST_COMP
/// Performance optimizations related to bit twiddling (http://www.graphics.stanford.edu/~seander/bithacks.html)
#define NQM_GAME_CORE_UTILS_OPTIMIZATIONS
/// CvWeightedVector's Top n Choices function now uses unsigned integers for indexes and choice numbers
#define AUI_WEIGHTED_VECTOR_FIX_TOP_CHOICES_USE_UNSIGNED
/// Optimizations and fixes to reduce distance check overhead
#define AUI_FIX_HEX_DISTANCE_INSTEAD_OF_PLOT_DISTANCE
/// Tweaks to make performance logs a bit more consistent and easier to read
#define AUI_PERF_LOGGING_FORMATTING_TWEAKS
/// Slightly increases stopwatch (performance counter) precision by performing the time delta subtraction bit before casting the result as a double
#define AUI_STOPWATCH_SUBTRACT_BEFORE_DELTA_CAST
/// Implements the missing iterator typedefs for BaseVector
#define AUI_FIX_FFASTVECTOR_BASEVECTOR_ITERATOR
/// Functions that called ints for variables used for indexes and choice numbers now call unsigned ints instead
#define AUI_FIX_FFASTVECTOR_USE_UNSIGNED
/// Optimized parts of functions responsible for updating plot vision code
#define AUI_PLOT_VISIBILITY_OPTIMIZATIONS
/// Optimizes loops that iterate over relative coordinates to hexspace
#define AUI_HEXSPACE_DX_LOOPS
/// CvUnit::canMoveOrAttackInto() no longer calls certain expensive calls twice (also improves pathfinder performance)
#define AUI_UNIT_FIX_CAN_MOVE_OR_ATTACK_INTO_NO_DUPLICATE_CALLS
/// CvUnit::canMoveInto() is optimized to not perform redundant checks for attack flag (also improves pathfinder performance)
#define AUI_UNIT_FIX_CAN_MOVE_INTO_OPTIMIZED
/// The object used to store danger values is changed to an array instead of an FFastVector
#define AUI_DANGER_PLOTS_FIX_USE_ARRAY_NOT_FFASTVECTOR
/// Units who are delayed dead will not be fetched by functions that get enemy defenders
#define AUI_PLOT_FIX_ENEMY_DEFENDER_GETTER_DOES_NOT_GET_DELAYED_DEAD
/// When the citizen manager reallocates all citizens, it no longer goes through the costly process of calculating the worst plot multiple times
#define AUI_CITIZENS_FIX_DO_REALLOCATE_CITIZENS_NO_COSTLY_PLOT_REMOVAL
/// If a city is on manual specialist control, reallocate citizens will not remove any specialists before reassigning citizens
#define AUI_CITIZENS_FIX_DO_REALLOCATE_CITIZENS_OBEY_MANUAL_SPECIALIST_CONTROL
/// The function that gets the amount of different trading partners a player has will now use an array instead of a vector to avoid crash possibilities
#define AUI_TRADE_FIX_GET_NUM_DIFFERENT_TRADING_PARTNERS_USES_ARRAY
/// Fixes the fact that the game's Linear Congruential RNG is set to use constants that would require a modulus of 2^31 instead of ones that need 2^32 (I couldn't introduce a modulus step because Civ5's engine really dislikes modifications to the RNG system)
#define AUI_RANDOM_FIX_CONSTANTS_SET_TO_MODULUS_2_POW_32
/// Fixes a possible crash when exiting the game caused by heap corruption when deallocating CvGameLeagues due to misuse of an FStaticVector
#define AUI_LEAGUES_FIX_POSSIBLE_DEALLOCATION_CRASH
/// Fixes a possible crash when exiting the game caused by too few items in a FStaticVector
#define AUI_TRADE_FIX_POSSIBLE_DEALLOCATION_CRASH
/// Adds explicit clearing functions to certain destructors to make sure they are executed to avoid memory corruption
#define AUI_EXPLICIT_DESTRUCTION
/// Fixes the fact that an FStaticVector type containing objects with trivial constructors (i.e. they are "Plain Old Data" = POD) is treated as the vector type wouldn't be POD (improves stability and performance)
#define AUI_TRADE_FIX_FSTATICVECTOR_CONTENTS_ARE_POD
/// Eliminates an unneccessary loop and a few more steps from the function that stores a trade route's path into the trade route's data
#define AUI_TRADE_OPTIMIZE_COPY_PATH_INTO_TRADE_CONNECTION
/// Fixes a possible crash that happens when flavors are broadcast
#define AUI_FLAVORMANAGER_FIX_POSSIBLE_CRASH_ON_FLAVOR_BROADCAST
/// When CvCity's constructor is called, component objects of CvCity have their parent pointers set immediately when the components are constructed (improves stability)
#define AUI_CITY_FIX_COMPONENT_CONSTRUCTORS_CONTAIN_POINTERS
/// Visibility update is always triggered when a plot's visibility changes for a player, thus fixing situations like purchasing a plot not updating sight immediately
#define AUI_PLOT_FIX_RESPONSIVE_VISIBILITY_UPDATE

/// Fixes the discrepancy where culture is not stored and calculated with hundredths in mind, which greatly messes up modifiers applied to it
#define AUI_PLAYER_FIX_JONS_CULTURE_IS_T100

/// Firaxis' original database-handling code had a weird way of allocating and destructing 2D arrays. It was not standard-compliant and possibly caused memory leaks. This has been rectified.
#define AUI_DATABASE_UTILITY_PROPER_2D_ALLOCATION_AND_DESTRUCTION
/// Makes UI combat prediction more representative of the actual combat that will take place when combat would tie
#define AUI_GAME_FIX_COMBAT_PREDICTION_ACCURATE_PREDICT_TIES
/// Extra combat strength from having heavy charge against a unit/plot that cannot retreat has been moved to CvUnit functions from the combat processing functions.
#define AUI_UNIT_FIX_HEAVY_CHARGE_BONUS_INTEGRATED_INTO_STACKS*/
/// Fixes a bug where Lua mods that use different functions for religion founding and enhancing could allow multiple players to enhance or found religions in simultaneous multiplayer with conflicting beliefs, resulting in religions enhancing/founding multiple times
// #define AUI_RELIGION_FIX_SIMULTANEOUS_ENHANCE_OR_FOUND_CAUSING_MULTIPLE
/*
/// Moved the check for available beliefs from CvUnit to CvReligion so that it always fires properly for non-pantheons
#define AUI_RELIGION_FIX_NO_BELIEFS_AVAILABLE_CHECK_FOR_NON_PANTHEON_MOVED
/// If a unit is dead, it cannot perform any mission (fixes cases where players could cause two missions to fire in multiplayer)
#define AUI_UNIT_MISSION_FIX_NO_MISSION_ON_DEATH

// Fixes to game bugs and New/Tweaked gameplay aspects ported from AuI

/// Yields are cached and processed after the player's turn completes, not before the player's turn starts
// ------ #define AUI_YIELDS_APPLIED_AFTER_TURN_NOT_BEFORE

/// Removes the cap of 8 range for unit sight; this was only needed because the for() loops weren't set up properly, resulting in too many unused cycles
#define AUI_PLOT_SEE_FROM_SIGHT_NO_MAXIMUM_SIGHT_RANGE
/// When choosing the top n choices from a weighted vector, choices with weight equal to the last choice are also included
#define AUI_WEIGHTED_VECTOR_FIX_TOP_CHOICES_TIE
/// In Hotseat (and now for all multiplayer modes), wars are now declared at the beginning of the AI's turn rather than at the beginning of the human player's turn
#define AUI_DIPLOMACY_AI_FIX_WAR_DECLARATION_IN_MULTIPLAYER*/

/*/// Disables the check for whether a unit is currently embarked for triggering Denmark's UA, so the pathfinder can use it properly
#define AUI_UNIT_MOVEMENT_FIX_BAD_VIKING_DISEMBARK_PREVIEW

/// The allows water walk check is fixed to no longer trigger if water walk improvements are not built adjacent to each other
#define AUI_UNIT_MOVEMENT_FIX_BAD_ALLOWS_WATER_WALK_CHECK*/
/*/// Changes the code that activates "We are Family" to work properly. Since achievements are disabled for modded games, this only works if the mod is turned into mock DLC
#define AUI_ACHIEVEMENT_FIX_RELIGION_WE_ARE_FAMILY_WORKING
/// Promotions that grant air combat bonuses are now allowed for units with no ability for air combat if the promotion also grants the ability for air combat
#define AUI_UNIT_FIX_ALLOW_COMBO_AIR_COMBAT_PROMOTIONS*/

/*/// Fixes the influence cost calculator function to only enable the reuse pathfinder flag when it wouldn't result in incorrect data
#define AUI_MAP_FIX_CALCULATE_INFLUENCE_DISTANCE_REUSE_PATHFINDER*/

/*/// Fixes base heal mod from players not actually increasing base healing
#define AUI_UNIT_FIX_BASE_HEAL_MOD
/// If a plot's feature is ignored when calculating the yield of a tile, this also extends to any yield changes based on the working city
#define AUI_PLOT_FIX_GET_YIELD_WITH_BUILD_IGNORE_FEATURE_EXTENDS_TO_CITY
/// Fixes the poor setup of calculating the potential gains of having an improvement on a plot with a certain road
#define AUI_PLOT_FIX_GET_YIELD_WITH_BUILD_IMPROVEMENT_WITH_ROUTE
/// If the plot's current route is pillaged, the extra yield from the route is no longer factored into the function
#define AUI_PLOT_FIX_IMPROVEMENT_YIELD_CHANGES_CATCH_PILLAGED_ROUTE
/// Fixes AI Shoshone Pathfinders not getting any goody hut bonuses (TODO: have AI Shoshone actually choose their goody hut bonus instead of getting a random one)
#define AUI_PLAYER_FIX_GOODY_HUT_PICKER
/// Fixes the fact that the function to get a great general stacked with the unit does not detect great admirals (for naval units)
#define AUI_UNIT_FIX_GET_STACKED_GREAT_GENERAL_WORKS_WITH_ADMIRAL
/// Fixes badly set up damage modifier checks (eg. Friendly lands modifier got applied twice to melee attackers, Friendly/Enemy territory bonus for ranged units applied based on plot target instead of unit plot)
#define AUI_UNIT_FIX_BAD_BONUS_STACKS
/// Implements the missing getter for the enemy defender based on the unit in question (rather than the player); this is important for hidden nationality units
#define AUI_PLOT_GET_VISIBLE_ENEMY_DEFENDER_TO_UNIT

/// Fixes the bug where order-specific hammer bonuses would go into overflow for an order that may not be eligible for those bonuses
 // ------ #define AUI_CITY_FIX_DO_PRODUCTION_NO_OVERFLOW_EXPLOIT 

#ifndef AUI_YIELDS_APPLIED_AFTER_TURN_NOT_BEFORE
/// If a city grows or starves a population, it will add any difference in food production after the change to its food supply. Among other things, this means a) the food yields earned by new citizens are evaluated just like all other yields, and b) the food consumption of the new citizen is taken into account on the turn the citizen is added
#define AUI_CITY_FIX_DO_GROWTH_USE_FOOD_AFTER_POP_CHANGE
/// Food and production earned from a newly constructed building is added into the city's food and production pools, just like how new buildings add onto all other yields the turn they are constructed
#define AUI_CITY_FIX_DO_PRODUCTION_CONSIDER_FOOD_HAMMERS_FROM_NEW_BUILDING
#endif
/// Domain modifiers to trade route yields now stack multiplicatively with other modifiers instead of additively. Among other things, this fixes Iron Curtain giving a lower-than-expected bonus to naval trade routes
#define AUI_TRADE_FIX_CONNECTION_VALUE_MULTIPLICATIVE_STACKING_DOMAIN_MODIFIERS*/

/*/// Fixes the bug where a low beaker yield put into an RA would result in an artificially lower beaker reward
#define AUI_DEAL_FIX_ACCURATE_EARLYGAME_RESEARCH_AGREEMENT_YIELDS

/// Automating a unit no longer resets the turn timer (from theCAndeMan)
//#define NQM_GAME_FIX_TURN_TIMER_RESET_ON_AUTOMATION*/

/*/// Adds a few more things that are transferred to gifted units (GS beaker count, whether the unit is set up for ranged attacks, how many interceptions the unit has made) and removed some things being applied to gifted units (eg. gifted GP no longer triggers CS quests)
#define AUI_UNIT_FIX_GIFTED_UNITS_ARE_GIFTED_NOT_CLONED
/// Fixes a piece of code related to setting tiles around puppets to be worked by the puppet that not only did not work as intended, but could also have strange side-effects as well.
#define AUI_CITY_FIX_PUPPET_WORKED_PLOT_OVERRIDE
/// Units that are marked for death no longer generate a ZoC (from RushSecond)
#define AUI_UNIT_MOVEMENT_FIX_DELAYED_DEATH_UNITS_GENERATE_ZOC

/// Fixed the fact that some player-based modifiers to research costs are all only aesthetic, i.e. their only effect before would be to increase the number the UI displays for tech cost (they didn't actually modify tech cost)
#define AUI_TECH_FIX_PLAYER_BASED_RESEARCH_COST_ONLY_AESTHETIC

/// Civilian units won't even start attempting to path to attack a tile. This should hopefully also fix the occasional problem of civilian units not wanting to move to a specific tile
#define AUI_UNIT_MISSION_FIX_CONTINUE_MISSION_CIVILIANS_DONT_ATTEMPT_ATTACK
/// Players are allowed to research if they own a city, they no longer need to found one before researching is unlocked (helps with Germany challenge on maps with CS's)
#define AUI_PLAYERTECH_FIX_CAN_RESEARCH_WITH_NO_FOUNDED_CITY*/

/*/// Hovering units will no longer embark on shallow water, but will embark in deep water
#define AUI_UNIT_FIX_HOVERING_EMBARK

/// Research overflow will no longer double-dip with research modifiers (once for the tech that generated it, once for the tech for which it is used). This also stops overflow from multiplying in ways that are exploitable and needed a band-aid fix from Firaxis.
#define AUI_PLAYER_FIX_NO_RESEARCH_OVERFLOW_DOUBLE_DIP
/// When war is manually declared by a player against another (i.e. not through a defensive pact), if the defender has any trade routes to the attacker, those trade routes get cancelled instead of destroyed. This applies to both sides of a Defensive Pact DoW.
#define NQM_TEAM_TRADE_ROUTES_CANCELLED_NOT_DESTROYED_FOR_WAR_DEFENDER_ON_DOW

/// If an air unit on intercept duty falls to at or below (value) HP after suffering an air sweep, it will get "knocked out" of intercept mode. This notifies human players of interceptors on low HP and hopefully stops interceptors with multiple intercepts per turn from getting killed from full health by two air sweeps, which stops increased intercepts per turn from being a death sentence
#define NQM_UNIT_COMBAT_WITHDRAW_INTERCEPT_AFTER_SWEEP_IF_AT_OR_BELOW_TARGET_HEALTH (50)
/// City-states are banned from building and capturing settlers outright (latter could previously not work), instead of the game relying on mishmash of flavors
#define AUI_PLAYER_FIX_ENSURE_NO_CS_SETTLER

/// Fixes the fact that game speed modifiers are applied twice to units that can blast tourism, i.e. Great Musicians; also fixes other, more rare bugs related to tourism blast strength (credits to FilthyRobot for finding the bug)
#define AUI_UNIT_FIX_NO_DOUBLE_SPEED_MODIFIER_FOR_TOURISM_BLAST

/// Adds an in-game toggleable option that allows players to gift their capitol; this is an alternative to voting someone irrelevant, especially because AIs can also do this, making it easier to conquer a player who already left because they became irrelevant
#define AUI_DEAL_ALLOW_CAPITOL_GIFTING
/// Changes a few lines of code so that only settlers are banned for Venice, settling as a whole is not banned (so they can have a separate settling unit)
#define AUI_PLAYER_FIX_VENICE_ONLY_BANS_SETTLERS_NOT_SETTLING
#ifdef AUI_PLAYER_FIX_VENICE_ONLY_BANS_SETTLERS_NOT_SETTLING
/// If a unit can both found and found abroad, Venice will only be able to use that unit to found abroad; this should allow Venice to not be overpowered with Exploration
#define NQM_UNIT_LIMIT_VENICE_CONQUISTADOR_SETTLES
#endif
/// Puppet cities belonging to Venice do not get penalties to science, gold, and faith (they still get penalties to culture to offset the fact that they don't increase policy cost)
#define AUI_CITY_FIX_VENICE_PUPPETS_GET_NO_YIELD_PENALTIES_BESIDES_CULTURE
/// Units with the ability to retreat from melee combat will not do so if they are guarding a civilian unit.
#define AUI_UNIT_FIX_NO_RETREAT_ON_CIVILIAN_GUARD
/// Fixed cases where moving a friendly unit into the owner's city would pop up an attack city dialogue AND where cities could be radared in unrevealed tiles.
#define AUI_UNIT_FIX_CAN_MOVE_INTO_CITY_ATTACK_BLOCKER*/

/// Tournament tweaks
#define TOURNAMENT_TWEAKS

#ifdef TOURNAMENT_TWEAKS

/*GP CHANGES START*/
///
// #define GREAT_SCIENTISTS_NERF
/// Increasing faith purchase cost of first Great Scientist
#define FAITH_FOR_THE_FIRST_SCIENTIST
/// Separating great scientists, engineers, merchants and prophets
#define SEPARATE_GREAT_PEOPLE // NEW: Counts Born Great Presons
/// Free Great person is now actually free
#define FREE_GREAT_PERSON
/// Great Scientists now bulb for science at point of birth, not current science
#define NEW_SCIENTISTS_BULB
///
// #define NEW_WRITERS_CULTURE_BOMB
///
#define DISCOVER_AMONT_SCIENCE_MODIFIER
/// After second turn of Great Scientist appearance bulb amount slowly decrease to 0 every turn
#define DECREASE_BULB_AMOUNT_OVER_TIME
/// Increasing hurry production while engineering SS Parts
#define SPACESHIP_ENGINEERING
/// Separating Great People Counts
#ifndef SEPARATE_GREAT_PEOPLE
/// Separating Great Merchant Count
// #define SEPARATE_MERCHANTS
#endif
/// Restriction on Great People gifts
#define CANT_GIFT_GP
/*GP CHAGES END*/



/*UNITS CHANGES START*/
/// Gifted units can't attack if they have already attacked this turn
#define GIFTED_UNITS_ATTACK
/// Can nuke only if Special Tech is researched
#define TECH_ALLOWS_NUKING
///
#define UNIT_DIED_BY_NUKING_NOTIFICATIONS
/// Players can't gain more than 45 exp from combat with AI or Minors
#define LIMITATION_COMBAT_EXPERIENCE
/// Restriction on paradroping in the second half of timer
#define CAN_PARADROP_HALF_TIMER
/// Restriction on setting interseption in the second half of timer
#define CAN_SET_INTERCEPT_HALF_TIMER
///
#define MARINE_DISEMBARK_FOR_1_MP
/// Civilians haven't defense while embarked like Work Boats
#define NO_EMBARKED_CIVILIAN_DEFENSE
/// Disabling autoexploring and autoworking tiles
#define DISABLE_AUTOMOVES
/// Removing paradrop animation for better UI expirience
#define REMOVE_PARADROP_ANIMATION
/// Removing some civilans animations for better UI expirience
#define REMOVE_GAMEPLAY_UNIT_ACTIVATE_ANIMATION
/// Staring settler have extra move on turn 0
#define STARTING_SETTLER_EXTRA_MOVE
///
#define PROMOTION_INSTA_HEAL_LOCKED
/// Decreasing Foreign Legion curchase cost
#define FOREIGN_LEGION_COST_PURCHASE
/// Stop centering camera on turn start or selected unit's death
#define REMOVE_EXCESS_CAMERA_CENTERING
///
#define DISABLE_BUILD_REPAIR_ON_NON_FRIENDLY_TERRITORY
///
#define FIGHTER_FINISHMOVES_AFTER_INTERCEPTION
///
#define MADE_REBASE
///
#define REBASE_WITH_AIRPORTS
///
#define ATTACK_BONUS_VS_RANGED_MOUNTED
///
#define CAPTURE_RESTRICTION_AFTER_PARADROPPING
///
#define UNIT_NUKE_DEFENSE
///
#define TRADE_UNITS_DO_NOT_CAUSES_REPOSITION
///
#define AVOID_UNIT_SPLIT_MID_TURN
///
#define VISIBLE_NAVAL_CAN_BLOCK_TILES
///
#define FIX_WITHDRAW_WHILE_DEFENDING_CIVILIAN
///
#define UNIT_DISBAND_REWORK
///
#define FREE_UNIT_AT_STARTING_PLOT
///
#define FIX_JUMP_TO_NEAREST_CITY
///
#define MINES_ON_LUXES_AFTER_BRONZE_WORKING
///
#define NO_PILLAGE_HEAL_ON_NEUTRAL_LAND
///
#define PRIZE_SHIPS_ALWAYS_CAPTURE
///
#define ALLOW_HELICOPTER_WATERWALK
///
#define PLUNDERING_TRADE_ROUTE_DECREASES_MP
///
#define UPD_RECON_PLOT_IF_CARGO_MOVE
///
#define REMOVE_CONTROL_CYCLE_PLOT_UNITS
///
#define FAST_BUILD_PREREQ_TECH
///
#define PROMOTION_NO_UNHAPPINESS_PENALTY
///
#define UNIT_UPGRADE_NUM_RESOURE_USED_CHANGE
///
#define MONGOLIAN_KHAN_RANGE_INCREASE
///
#define FIX_DOMAIN_SEA_ZOC
///
#define TRADE_ROUTES_TARGET_TURNS 20
/*UNITS CHANGES END*/



/*TECHNOLOGIES AND RESEARCH CHANGES START*/
///
#define RES_AGR_COUNT
///
#define NO_PUPPET_TECH_COST_MOD
/// The discount to tech cost awarded for other teams already owning a specific tech can now be toggled via an in-game option
// #define AUI_TECH_TOGGLEABLE_ALREADY_KNOWN_TECH_COST_DISCOUNT
/// AI does not increase already known tech cost discount
#define HAS_TECH_BY_HUMAN
/// Autospending free technologies that was not spent during turn transition
#define AUTOMATICALLY_SPEND_FREE_TECHNOLOGIES
///
#define RA_LESS_TURNS
///
#define FUTURE_TECH_RESEARCHING_BONUSES
///
#define NEW_NUM_CITIES_RESEARCH_COST_MODIFIER
///
#define MEDIAN_TECH_PERCENTAGE_DOES_NOT_AFFECT_KOREA
///
#define UI_TECH_KNOWN_COUNT
/*TECHNOLOGIES AND RESEARCH CHANGES END*/



/*POLICIES CHANGES START*/
/// Free courthouses are no longer removed when puppeting a city
#define AUI_CITY_FIX_DO_CREATE_PUPPET_FREE_COURTHOUSES_KEPT
/// Fixing double aqueduct bug
#define AQUEDUCT_FIX
///
// #define OWED_FOOD_BUILDING
/// Freedom players can purcahse SS Parts only in cities with 5+ population
#define SS_PART_PURCHASE_RESTRICTION
///
#define SHARED_IDEOLOGY_TRADE_CHANGE
/// Special Policy Grants 1 additional Delegate in the World Congress for every allied City-State (up to MaxExtraVotesFromMinors additional Delegates)
#define POLICY_MAX_EXTRA_VOTES_FROM_MINORS
///
#define POLICY_EXTRA_VOTES
/// Treaty Organization One time increase of 45 Influence with City-States
#define POLICY_MINOR_INFLUENCE_BOOST
/// Adopting all policies in the Liberty tree will decrease Unhappiness from each city by 1.
// #define LIBERTY_FINISER_LESS_UNHAPPINESS_PER_CITY
/// Biology, Electrisity, Steam Power and Dynamite triggers ideology choise
// #define NEW_IDEOLOGY_TRIGGER
///
#define MORE_FREE_IDEOLOGY_TENETS
///
#define POLICY_BRANCH_UNLOCKING_TURN
///
#define POLICY_BRANCH_NOTIFICATION_LOCKED
/// The game will now send a “can adopt policy” notification if you get enough culture for a policy mid-turn
#define UPDATE_CULTURE_NOTIFICATION_DURING_TURN
///
#define PATRONAGE_FINISHER_REWORK
///
#define POLICY_ALLOWS_GP_BUYS_FOR_GOLD
///
#define POLICY_ATTACK_BONUS_TURNS_RANGE_MODIFIER
///
#define POLICY_DO_TECH_FROM_CITY_CONQ
///
#define POLICY_GOLD_PER_CS_FRIENDSHIP
///
#define POLICY_GOLDEN_AGE_YIELD_MOD
///
#define POLICY_NO_CULTURE_SPECIALIST_FOOD
///
#define POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
#ifdef POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE
#define NUM_SPECILIST_TYPES 7
#endif
///
#define FIX_POLICY_FREE_RELIGION
///
#define LEARNED_SOCIETY_SCIENCE_MOD
///
#define POLICY_FAITH_COST_MODIFIER_ALL_PURCHASES
///
#define POLICY_MINORS_GIFT_UNITS
///
#define POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES
///
#define POLICY_NO_CARGO_PILLAGE
///
#define POLICY_SCIENCE_PER_X_FOLLOWERS
///
#define POLICY_GLOBAL_POP_CHANGE
///
#define POLICY_HAPPINESS_PER_CITY
///
// #define UNIVERSAL_SUFFRAGE_TWO_THIRD_UNHAPPINESS
///
#define PENALTY_FOR_DELAYING_POLICIES
///
#define POLICY_GREAT_WORK_HAPPINESS
///
#define HONOR_CULTURE_CAP
#ifdef HONOR_CULTURE_CAP
#define CULTURE_CAP 30
#endif
///
// #define FLOURISHING_OF_ARTS_REWORK
///
#define POLICY_GREAT_WORK_TOURISM_CHANGES
///
#define POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP
///
#define POLICY_FREE_DEFENSIVE_BUILDINGS
///
#define POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD
///
#define POLICY_BUILDING_CLASS_FOOD_KEPT
///
#define FIX_POLICY_BUILDING_CLASS_CULTURE_CHANGE_UI
///
#define NEW_NUM_CITIES_POLICIES_COST_MODIFIER
///
#define FIX_POLICY_CULTURE_PER_GARRISONED_UNIT
///
#define POLICY_EXTRA_SPIES
///
#define POLICY_SPY_MINOR_PER_TURN_INFLUENCE
///
#define POLICY_FOE_TOURISM_MODIFIER
///
#define POLICY_HAPPY_TOURISM_MODIFIER
///
#define POLICY_ONLY_INTERNAL_TRADE_ROUTE_YIELD_MODIFIER
///
#define POLICY_BUILDINGCLASS_TOURISM_CHANGES
///
#define POLICY_CAPITAL_CULTURE_MOD_PER_DIPLOMAT
///
#define POLICY_EXTRA_TRADE_ROUTES
///
#define POLICY_HAPPINESS_PER_TRADE_ROUTE_TO_CAP
///
#define POLICY_HAPPINESS_PER_TRADE_ROUTE_TO_MINOR
///
#define POLICY_EXTRA_VOTES
///
#define POLICY_LEAGUE_SESSION_YIELD_BOOST_PER_DELEGATE
///
#define POLICY_NUM_CITIES_FREE_CULTURE_BUILDING_ONLY_CULTURE_BUILDINGCLASS
/*POLICIES CHANGES END*/



/*TOURISM CHANGES START*/
///
#define NO_OPEN_BOARDERS_TOURISM_MOD
///
#define TOURISM_NO_SCIENCE_FROM_TRADE_ROUTES
///
#define SCIENCE_FROM_INFLUENCED_CIVS
///
#define DEFENSE_AGAINST_INFLUENCED_CIVS
///
#define TOURISM_BONUS_DIPLOMAT
///
#define REWORK_PUBLIC_OPINION
///
#define NO_SPEED_MOD_FOR_TOURISM_BLAST
///
// #define BLAST_TOURISM_ON_OWNERS_LAND
/*TOURISM CHANGES END*/



/*RELIGION CHANGES START*/
// Allow lake feature yield boosts from religion - this is a huge cheat so I don't have to code a whole new table for "fake features" which 
// is what lakes are categorized under. In NQMod, ice has no belief bonuses, and so I'm overriding FEATURE_ICE to be used for lakes instead.
#define NQ_LAKE_BELIEF_BONUSES
///
#define BELIEF_PEACE_LOVING_PER_PEACE_FULL_FOLLOWERS
///
#define BELIEF_WORLD_CHURCH_PER_FOLLOWERS
///
#define BELIEF_INTERFAITH_DIALOGUE_PER_FOLLOWERS
///
#define BELIEF_PILGRIMAGE_PER_CITY
///
#define BELIEF_DANCE_AURORA_NERF
/// allow religion one-shots (like deus vult, religious settlements, dharma)
#define NQ_ALLOW_RELIGION_ONE_SHOTS
/// allow golden ages from beliefs
#define NQ_GOLDEN_AGE_TURNS_FROM_BELIEF
// extra trade routes from beliefs
#define BELIEF_EXTRA_TRADE_ROUTES
///
#define MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD
///
#define PROPHET_CAN_FOUND_EXTRA_RELIGION
///
#define UNITY_OF_PROPHETS_EXTRA_PROPHETS
///
#define REFORMATION_BELIEFS_ONLY_FOR_FOUNDERS
///
#define GP_RATE_MODIFIER_FROM_BELIEF
///
#define UNDERGROUND_SECT_REWORK
///
#define BELIEF_TO_GLORY_OF_GOD_ONE_GP_OF_EACH_TYPE
///
#define BELIEF_GREAT_WORK_YIELD_CHANGES
///
#define BELIEF_SPECIALIST_YIELD_CHANGES
///
#define BELIEF_HURRY_MODIFIERS
///
#define BELIEF_MESSENGER_OF_GODS_FIX
///
#define RELIGIOUS_TOLERANCE_DOUBLES_OWNER_PANTHEON
///
#define NEW_PAPAL_PRIMACY
///
#define GODDESS_LOVE_FREE_WORKER
///
#define GOD_SEA_FREE_WORK_BOAT
///
#define SACRED_WATERS_FRESH_WATER
///
#define RELIGIOUS_UNITY_CS_BONUS
///
#define MIN_FAITH_NEXT_PANTHEON_UPDATES_ONCE_PER_TURN
///
#define FIX_NO_RELIGION_MAJORITY
///
#define RELIQUARY_REWORK
///
#define GP_EXPENDED_GA
///
#define NEW_BELIEF_PROPHECY
///
#define BELIEF_HOLY_ORDER_EXPANSION
///
#define SPREAD_MODIFIER_AFTER_TECH
///
#define UI_GET_PRESSURE
///
#define BELIEF_HALF_FAITH_IN_CITY
/*RELIGION CHANGES END*/



/*UA CHANGES START*/
/// Fixes Iroquois' UA so friendly forest tiles will now connect with road tiles!
#define AUI_UNIT_MOVEMENT_IROQUOIS_ROAD_TRANSITION_FIX
///
// #define BYZANTIUM_CAN_ALWAYS_FOUND_RELIGION
/// India have 2 extra Happiness
#define INDIA_HAPPINESS_BONUS
///
#define NEW_BYZANTIUM_UA
///
#define NEW_OTTOMAN_UA
/// Adding source for Trait_BuildingClassHappiness table
#define TRAIT_GET_BUILDING_CLASS_HAPPINESS
///
#define NEW_VENICE_UA
///
#define MOROCCO_UA_REWORK
///
#define ETHIOPIA_UA_REWORK
///
#define RUSSIA_UA_REWORK
///
#define PORTUGAL_UA_REWORK
///
#define DENMARK_UA_REWORK
///
#define INDONESIA_UA_REWORK
///
#define CARTHAGE_UA_REWORK
///
#define ROME_UA_REWORK
///
#define CHINA_UA_REWORK
///
#define ASSYRIA_UA_REWORK
///
#define MONGOL_UA_REWORK
#ifdef MONGOL_UA_REWORK
#define MONGOL_CS_BULLY_MOD 30
#endif
///
#define SWEDEN_UA_REWORK
///
#define BRAZIL_UA_REWORK
///
#define SIAM_UA_REWORK
///
#define GREECE_UA_REWORK
///
#define JAPAN_UA_REWORK
///
#define FRANCE_UA_REWORK
///
#define AUSTRIA_UA_REWORK
#ifdef AUSTRIA_UA_REWORK
#define AUSTRIA_UA_CONTACT_GOLD 40
#endif
///
#define CELTS_UA_REWORK
///
#define IROQUOIS_UA_REWORK
/// Golden age combat bonus modifier now applies only during attack
#define GOLDEN_AGE_ATTACK_BONUS_MODIFIER
///
#define TRAIT_ALWAYS_ALLOWED_INNER_TRADE_ROUTES
///
#define TRAIT_GOLD_FOR_LUXURY_EXPORT
///
#define NO_BARN_FOR_INCA
/*UA CHANGES END*/



/*WLTKD CHANGES START*/
///
#define WLKTD_STARTS_IF_NO_VALID_RESOURCES_TO_DEMAND
/// We Love The King Day resource requested changes if it takes too long to fulfill
#define NQ_WLTKD_RESOURCE_DEMAND_EXPIRES
///
#define NEW_WLTKD
///
#define WLTKD_DO_PICK_RESOURCES_IN_BORDERS_ONLY
/*WLTKD CHANGES END*/



/*AI CHANGES START*/
/// AI players will no longer spread their religion to other human players' cities
#define NQM_AI_GIMP_NO_RELIGION_SPREAD
/// AI players will no longer attempt to build any world wonders or world projects
#define NQM_AI_GIMP_NO_WORLD_WONDERS
/// AI players will always accept white peace offered by human players (this is a lot easier to implement than banning DoWs outright, which would mess with everything that relies on operations and sneak attacks)
#define NQM_AI_GIMP_ALWAYS_WHITE_PEACE
/// AI players will not build units that can settle. Also disables "expansion" economic strategies
#define NQM_AI_GIMP_NO_BUILDING_SETTLERS
///
#define CAN_FOUND_OR_ENHANCE_OR_SPREAD_REL_ONLY_HUMAN
///
#define NO_AI_ALLYING_CS
///
#define AI_CANT_ADOPT_IDEOLOGY
///
#define AI_CANT_DECLARE_WAR
///
#define NO_AI_PROJECTS
///
#define NO_AI_TRADE_ROUTES
///
#define NO_TRADE_ITEMS_WITH_AI
///
#define NO_AI_VOTES
///
#define AI_PEACE_TURNS
///
#define DO_CANCEL_DEALS_WITH_AI
///
#define DO_EXTRACT_AI_SPIES
///
#define AI_CULTURE_RESTRICTION
///
#define NO_STEALABLE_TECH_LIST_FROM_AI
///
#define AI_CANT_COUP
///
#define FINISH_KICKED_PLAYER_MOVES
///
#define AI_CANT_DO_CULTURE_BOMB
/*AI CHANGES END*/



/*RADARING CHANGES START*/
///
// #define ASTAR_AI_CONTROL_FIX_RADAR
/// Fixes radar (Delnar: first bit was covered by GJS, remaining bits are now also covered) 
#define AUI_ASTAR_FIX_RADAR
/// Fixes rarer cases of radar
#define AUI_UNIT_FIX_RADAR
/// Fixes cases of indirect radaring via ZOC.
#define AUI_UNIT_MOVEMENT_FIX_RADAR_ZOC
/*RADARING CHANGES END*/



/*CITY-STATES CHANGES START*/
///
#define NQ_NEVER_PUSH_OUT_OF_MINORS_ON_PEACE
// city states will no longer declare peace if influence is less than -50
// #define NQ_PEACE_BLOCKED_IF_INFLUENCE_TOO_LOW
///
#define DECREASE_INFLUENCE_IF_BULLYING_SOMEONE_WE_ARE_PROTECTING
///
#define NO_FAITH_FROM_MEETING_CS
///
#define PEACE_BLOCKED_WITH_MINORS
// coup chances now use base influence instead of effective influence (which would be -60 during war)
#define NQ_COUP_FORMULA_USES_BASE_FRIENDSHIP_NOT_EFFECTIVE_FRIENDSHIP
///
#define CANNOT_LIBERATE_GIFTED_CS
///
#define WORKER_BULLY_RESRICTION
///
#define NEW_CITY_STATES_TYPES
// New City States pseudo-enums
#define MINOR_CIV_TRAIT_SCIENTIFIC 5
#define MINOR_CIV_TRAIT_MANUFACTORY 6
// IMPORTANT
// Must be defined only ONCE and account for all MinorCivTraitTypes additions
#define NUM_MINOR_CIV_TRAIT_TYPES 7
///
#define ALL_QUESTS_GET_END_TURN
///
#define CS_ALLYING_WAR_RESCTRICTION
#ifdef CS_ALLYING_WAR_RESCTRICTION
#define CS_ALLYING_WAR_RESCTRICTION_TIMER 60.f
#endif
///
#define NEW_BULLY_METRICS
///
#define CS_CANT_BUILD_EARLY_WORKERS
#ifdef CS_CANT_BUILD_EARLY_WORKERS
#define CS_EARLY_WORKERS_TURN 20
#endif
///
#define UPDATE_MINOR_BG_ICON_ON_UNIT_MOVE_OR_SET_DAMAGE
///
#define UPDATE_MINOR_TECHS_ON_LIBERATION
///
#define BUMP_UNITS_OUT_MINOR_LAND
///
#define QUESTS_SYSTEM_OVERHAUL
/*CITY-STATES CHANGES END*/



/*GOODIES CHANGES START*/
///
#define XP_RUINS_FIX
///
#define REMOVE_EARLY_CULTURE_RUINS
/// Goody hut messages now properly appear for all yields, even if there's no popup
#define AUI_PLAYER_FIX_RECEIVE_GOODY_MESSAGE
/// If the player receives a yield from a goody hut, floating text appears above the plot indicating the number and type of yields received
// #define AUI_PLAYER_RECEIVE_GOODY_PLOT_MESSAGE_FOR_YIELD
///
#define INCREASE_NUM_GOODIES_REMEMBERED
///
#define POP_RUIN_FOOD_NOT_POPULATION
///
#define FLAT_SCIENCE_FROM_TECH_RUIN
///
#define EXPIRIENCE_RUIN_SET_MOVEMENT_PROMOTION
///
#define NEW_RUIN_EXPANSION
/*GOODIES CHANGES END*/



/*CITIES CHANGES START*/
/// Can't sell buildings before entering Info Era 
#define CANT_SELL_BEFORE_INFO_ERA
///
#define CANT_SELL_CHEAP_BUILDINGS_WITH_MODIFIER_FROM_POLICIES
/// Can't liberate cities for non-human players, can liberate city-states if only league leader
#define NEW_CITIES_LIBERATION
/// Recapturing a city originally owned by a player on the same team will properly no longer cause population losses or razed buildings
#define AUI_PLAYER_FIX_ACQUIRE_CITY_NO_CITY_LOSSES_ON_RECAPTURE
/// Adds a new field for minimum city yield that adjust said yield when the city is on hills (currently -1)
#define NQM_YIELD_MIN_CITY_ON_HILLS_ADJUST
/// Time resistance of cities is half than normal
#define REDUCE_RESISTANCE_TIME
///
#define PRODUCTION_OVERFLOW_INTO_GOLD
/// Removed graphics of Spaceship Parts
#define SPACESHIP_GRAPHICS
/// Can't trade damaged cities
// #define TRADE_CITIES_ONLY_FULL_HP
/// Puppet cities and cities with automated production will no longer accidentally trigger the production notification
#define AUI_CITY_FIX_PUPPET_CHOOSE_PRODUCTION_NOTIFICATION
/// Adds a bunch of extra checks to the production notification invalidator so that it gets invalidated properly in more cases (e.g. when the city's owner changes)
#define AUI_NOFITICATIONS_FIX_PRODUCTION_NOTIFICATION_REMOVAL_EXTRA_CHECKS
///
#define LUA_METHOD_IS_INDUSTRIAL_ROUTE_TO_CAPITAL
///
#define PREVENT_UNCAPPED_OVERFLOW
///
#define FREE_BUILDINGS_COUNTS_AS_REAL_ON_CITY_ACQUIRE
/// 
#define DESTROYING_MOST_EXPENSIVE_BUILDINGS_ON_CITY_ACQUIRE
///
#define CHANGE_CITY_ORIGINAL_OWNER
///
#define UPDATE_UNIT_PROMOTIONS_ON_ACQUIRED_PLOT
/*CITIES CHANGES END*/



/*BUILDINGS CHANGES START*/
/// Oxford University is available before entering Atomic Era
#define NO_OXFORD_AFTER_ATOM
/// Oxford University and National Intelligence Agency can be build once per game
#define CAN_BUILD_OU_AND_NIA_ONLY_ONCE
///
#define BUILDING_CLASS_YIELD_CHANGES
///
#define NEW_FACTORIES
///
#define BUILDING_CITY_RANGE_MODIFIER
///
#define BUILDING_CITY_EXTRA_HEAL
///
#define BUILDING_CITY_EXTRA_ATTACK
///
#define BUILDING_IMPROVEMENT_YIELD_CHANGE
///
#define BUILDING_FAITH_TO_SCIENCE
///
#define BUILDING_INCREASE_BONUSES_PER_ERA // TODO: Remake xml
///
#define BUILDING_CITY_TILE_WORK_SPEED_MOD
///
#define BUILDING_HURRY_COST_MODIFIER
///
#define BUILDING_RIVER_GOLD
///
#define BUILDING_GROWTH_GOLD
///
#define BUILDING_BAKTUN_GOLD_AGE_POINTS
///
#define BUILDING_SCIENCE_PER_X_POP
///
#define BUILDING_CULTURE_PER_X_ANCIENCT_BUILDING
///
#define BUILDING_NEAR_MOUNTAIN_YIELD_CHANGES
///
#define BUILDING_DOUBLE_DEFENSE_NEAR_MOUNTAIN
///
#define BUILDING_NO_HOLY_CITY_AND_NO_OCCUPIED_UNHAPPINESS
///
#define BUILDING_NEARBY_ENEMY_DAMAGE
///
#define BUILDING_NAVAL_COMBAT_MODIFIER_NEAR_CITY
///
#define BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE
///
#define BUILDING_HAPPINESS_FOR_FILLED_GREAT_WORK_SLOT
///
#define BUILDING_FOOD_YIELD_MODIFIERS_GROTH
///
#define BUILDING_FOOD_BONUS_IF_NO_CITIES_AROUND
///
#define BUILDING_LOCAL_CITY_CONNECTION_TRADE_ROUTE_MODIFIER
///
#define BUILDING_NON_AIR_UNIT_MAX_HEAL
///
#define BUILDING_DOUBLE_PANTHEON
/*BUILDINGS CHANGES END*/



/*BUGS FIXES START*/
/// Fixes the bug where building a new improvement on a tile with a pillaged improvement keeps the tile marked as pillaged
#define AUI_PLOT_FIX_PILLAGED_PLOT_ON_NEW_IMPROVEMENT
/// Fixes the fact that in simultaneous turns multiplayer, barbarians can spawn and then move units in the same turn.
#define AUI_GAME_FIX_MULTIPLAYER_BARBARIANS_SPAWN_AFTER_MOVING
/// Fixes a possible null pointer dereferences in FoundPantheon()
#define AUI_RELIGION_FIX_FOUND_PANTHEON_NULL_POINTER_DEREFERENCE
/// Planes will take damage and Guided Missiles will dissapear when war is declaring by Air Strike
#define FIX_AIR_STRIKE_WHEN_DECLARING_WAR
// Fixes incorrect plot distance calculation for wrapped maps
#define GAMECOREUTILS_FIX_PLOT_DISTANCE
/// Fix bug of SetImprovementType when caused by placing Citadels on hided resource on enemy's territory
#define FIX_SET_IMPROVEMENT_TYPE
///
#define FIX_SET_IMPROVEMENT_PILLAGED_HAPPINESS_UPDATE
///
#define FIX_DO_ATTACK_SUBMARINES_IN_SHADOW_OF_WAR
///
#define INVISIBILITY_OF_NUCLEAR_MISSILESS_ON_SUBMARINES
/// Don't draw other players notification dots on the minimap
#define MINIMAP_FIX_DRAW_ONLY_SUITABLE_NOTIFICATIONS
///
#define FIX_CLOSE_EMBASSY_AT_TEAM_REVEALING_PLOTS
/// Adds missing sanity checks; prevents unit/resource duping inside laggy game tick
#define NET_FIX_SINGLE_USE_ABILITY_DUPE
/// Fixed the bug where the production bonus from having a railroad connecting a city to the capital is not removed if the railroad connection is broken (credits to Wr4ith pointing this out after having researched the "weirdness" behind harbors, railroads, and the railroad bonus)
#define AUI_CITY_FIX_UPDATE_RAILROAD_CONNECTION_ALLOW_REMOVAL
/// Fixes already corrupted saves leading to turn skip in simultaneous mode.
#define AUTOSAVE_FIX_PREVENT_TURN_SKIP
///
#define FIX_TOURISM_BLAST_FROM_POLICIES
///
#define CITIZENS_CITY_OVERRIDE_BUG_FIX
///
#define CORRECT_ATTACK_CITY_STRENGTH
/// Vanilla typo
#define FIX_CVTEAM_BITWISE_OR_MISUSE
/// Don't show the "choose tech" notification every time the active player bulbs a scientist (setHasTech still does that whenever is needed)
#define FIX_SCIENTIST_CHOOSE_TECH_NOTIFICATION
/// Adds missing sanity checks for some city tasks
#define NET_FIX_EXPLOITABLE_CITY_TASKS
///
#define FIX_AIR_ATTACK_VS_EMBARKED
///
#define FIX_MAX_EFFECTIVE_CITIES
///
#define FIX_RANGE_DEFENSE_MOD
/// if free tech notification is present, mark choose tech notification as redundant
#define FIX_REDUNDANT_CHOOSE_TECH_NOTIFICATION
///
#define FIX_REMOVE_EXPIRED_FREE_TECH_NOTFICATION
///
#define FIX_LOCALIZATION_DESC
///
#define FIX_UPDATE_CITY_VIEW_FREE_BUILDING
///
#define FIX_EXCHANGE_PRODUCTION_OVERFLOW_INTO_GOLD_OR_SCIENCE
///
#define FIX_BAZAAR_DOUBLE_RESOURCE_ONCE
///
#define FIX_ALL_UPGRADES_AVAILABLE
///
#define FIX_IS_NATIONAL_WONDER_CLASS
///
#define FIX_CIVILLIANS_DROP_CITIZENS_FROM_BLOCKED_TILES
///
#define FIX_POLICY_FREE_BUILDING_ON_CONQUEST_INCLUDES_UNIQUE_BUILDINGS
///
#define FIX_PURCHASE_ADMIRAL_IN_LAND__CITIES
///
#define FIX_DO_CANCEL_DEALSE_ON_WAR_DECLARATION
///
#define FIX_BARBARIAN_NULL_PLOT_POINTER
///
#define FIX_SIGHT_UPDATE_AFTER_REMOVING_FEATURE
/*BUGS FIXES END*/



/*TURN AND TIMER CHANGES START*/
/// Randomizes the order in which player turns activate in simultaneous mode. E.g. this makes it so that the host no longer wins wonder races against all other players if they finish a wonder the same turn as another player.
#define NQM_GAME_RANDOMIZE_TURN_ACTIVATION_ORDER_IN_SIMULTANEOUS
///
#if defined (NQM_GAME_RANDOMIZE_TURN_ACTIVATION_ORDER_IN_SIMULTANEOUS) && defined (AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER)
// #define AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER_OPTION
#endif
/// New option that allows custom turn timer settings to multiply/divide the default turn times by a certain amount instead of forcing turn times to be the custom amount
#define AUI_GAME_RELATIVE_TURN_TIMERS
///
#define TURN_TIMER_RESET_BUTTON
///
#define TURN_TIMER_PAUSE_BUTTON
/// Prevent timer mid-turn fluctuations - update it once per turn
#define GAME_UPDATE_TURN_TIMER_ONCE_PER_TURN
/// Player can move only one unit while MP turn is loading
#define GAME_ALLOW_ONLY_ONE_UNIT_MOVE_ON_TURN_LOADING
///
#define DO_TURN_CHANGE_ORDER
/// move autosave to the end of turn
#define AUTOSAVE_END_OF_TURN
/// Game.GetTurnTimeElapsed() returns elapsed time in milliseconds
#define LUAAPI_GET_TURN_TIME_ELAPSED
///
#define AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL
/*TURN AND TIMER CHANGES END*/



/*WONDERS CHANGES START*/
///
#define TEMPLE_ARTEMIS_NO_YIELD_MOD_BUT_GROWTH
///
#define TAJ_MAHAL_STARTS_GA_NEXT_TURN
///
#define KREMLIN_GLOBAL_MOD
///
#define GREAT_FIREWALL_DROPS_OUT_SPIES
///
#define GREAT_FIREWALL_REDUCE_HALF_INTERNET_BONUS
/*WONDERS CHANGES END*/



/*LEAGUE CHANGES START*/
/// If multiple civs have are eligible to found the league, choose a random one instead of the one with the highest slot
#define AUI_VOTING_RANDOMIZED_LEAGUE_FOUNDER
///
#define DIPLO_VICTORY_VOTING
///
#define CAN_PROPOSE_ENACT_UPDATES_ONCE_PER_SESSION
///
#define FINISH_LEAGUE_SESSION_EVEN_IF_SOMEONE_HAS_VOTES
///
#define EXTENDED_PLAYER_EMBARGO_EFFECTS
///
#define ASSIGN_SECOND_PROPOSAL_PRIVILEGE
///
// #define CHANGE_HOST_IF_DISCONNECTED
///
#define LEAGUE_PROJECT_REWARD_TIER_3_THRESHOLD
///
#define PROJECT_COST_PER_PLAYER_ROUND_TO_5
///
#define VOTES_FOR_MOST_RELIGION_FOLLOWERS
///
#define SHOW_NUCLEAR_NON_PROLIFERATION_ACTIVE_EFFECT
///
#define NUCLEAR_NON_PROLIFERATION_INCREASE_NUKES_COST
///
#define NEW_RESOLUTION_MEMBER_DISCOVERED_TECH_DISCOUNT
///
#define NEW_LEAGUE_RESOLUTIONS
///
#define FIX_PROJECTS_NOTIFICATIONS_PERCENTAGE
/*LEAGUE CHANGES END*/



/*ESPIONAGE CHANGES START*/
///
#define CANT_STEAL_CLASSICAL_ERA_TECHS
///
#define BUILD_STEALABLE_TECH_LIST_ONCE_PER_TURN
#ifdef BUILD_STEALABLE_TECH_LIST_ONCE_PER_TURN
///
#define ESPIONAGE_SYSTEM_REWORK
#endif
///
#define DECREASE_GATHERING_INTEL_TIME_LENGTH
///
#define NEW_DIPLOMATS_MISSIONS
#ifdef NEW_DIPLOMATS_MISSIONS
#define ESPIONAGE_SCHMOOZE_LEVEL_UP_BASE_COST 40
#endif
///
#define AUTO_PEACE_WITH_MINOR_ON_COUP
/*ESPIONAGE CHANGES END*/



/*MP VOTING SYSTEM START*/
///
#define MP_PLAYERS_VOTING_SYSTEM
/// hashed pseudo-enums
#define NOTIFICATION_MP_IRR_PROPOSAL -300693023
#define NOTIFICATION_MP_CC_PROPOSAL -832529557
#define NOTIFICATION_MP_SCRAP_PROPOSAL 1628401511
#define NOTIFICATION_MP_REMAP_PROPOSAL 577315649
#define NOTIFICATION_MP_PROPOSAL_RESULT -1076480691
/*MP VOTING SYSTEM END*/

/*INGAME HOTKEY MANAGER START*/
// Adds UI options screen and internal methods to customize units/interface hotkeys
// Note: on lua side, GameInfo table still holds the original hotkeys, and GameInfoactions data is updated as soon as Game.UpdateActions() is called;
#define INGAME_HOTKEY_MANAGER
// Extended controls: pseudo-enums
#define CONTROL_CAMERA_MOVE_LEFT 44
#define CONTROL_CAMERA_MOVE_RIGHT 45
#define CONTROL_CAMERA_MOVE_FORWARD 46
#define CONTROL_CAMERA_MOVE_BACK 47
#define CONTROL_CAMERA_MOVE_OUT 48
#define CONTROL_CAMERA_MOVE_OUT_ALT 49
#define CONTROL_CAMERA_MOVE_IN 50
#define CONTROL_CAMERA_MOVE_IN_ALT 51
#define CONTROL_TOGGLE_HEX_GRID 52
#define CONTROL_TOGGLE_CHAT 53
// IMPORTANT
// Must be defined only ONCE and account for all ControlTypes additions
#define NUM_CONTROL_TYPES 54
/*INGAME HOTKEY MANAGER END*/


/*OTHER CHANGES START*/
/// Can't declare war for 20 turns before renaissance and for 10 turns else
#define NEW_DEFENSIVE_PACT
///
#define DEF_PACT_COUNT
///
#define GOLD_FOR_CIRCUMNAVIGATE
///
#define GET_REMOVE_FEATURE_FOOD
///
#define INCLUDE_WATER_IN_LAND_SCORE
///
#define MP_ALWAYS_QUICK_COMBAT_AND_MOVEMENT
///
#define REMOVE_DOF
///
#define CREATE_APOLLO_PROGRAM_WITH_CAP_FOUND
///
#define REVEAL_MAP_GAME_OVER
// Maintain backwards compatibility with older versions
// Starting from v7.0
#define SAVE_BACKWARDS_COMPATIBILITY
#ifdef SAVE_BACKWARDS_COMPATIBILITY
// Individual read/write components versioning
//
// 0-999 reserved for vanilla and other mods compatibility
// bump after any change to the corresponding read/write functions
// don't forget to mark changes here
//
// 1000: v7.0   (initial)
// 1001: v10.0  (adds GP_EXPENDED_GA, NEW_BELIEF_PROPHECY)
// 1002: v10.2  (adds ENHANCED_GRAPHS)
// 1003: v10.5  (adds BELIEF_HALF_FAITH_IN_CITY)
# define BUMP_SAVE_VERSION_BELIEFS 1003
// 1000: v11.1c (initial)
# define BUMP_SAVE_VERSION_CITY_RELIGIONS 1000
// 1000: v7.0   (initial)
// 1001: v8.0   (adds BUILDING_CITY_EXTRA_ATTACK)
// 1002: v8.2   (adds eReligionFoundedHere for MISSIONARY_ZEAL_AUTO_RELIGION_SPREAD)
// 1003: v10.2  (adds BUILDING_IMPROVEMENT_YIELD_CHANGE, FIX_EXCHANGE_PRODUCTION_OVERFLOW_INTO_GOLD_OR_SCIENCE)
// 1004: v11.0  (adds BUILDING_FAITH_TO_SCIENCE, BUILDING_CITY_TILE_WORK_SPEED_MOD, BUILDING_HURRY_COST_MODIFIER, BUILDING_GROWTH_GOLD, BUILDING_SCIENCE_PER_X_POP, BUILDING_CULTURE_PER_X_ANCIENCT_BUILDING, BUILDING_NEAR_MOUNTAIN_YIELD_CHANGES, BUILDING_DOUBLE_DEFENSE_NEAR_MOUNTAIN, BUILDING_NO_HOLY_CITY_AND_NO_OCCUPIED_UNHAPPINESS, BUILDING_NEARBY_ENEMY_DAMAGE, BUILDING_CITY_EXTRA_HEAL, BUILDING_NAVAL_COMBAT_MODIFIER_NEAR_CITY, BUILDING_HAPPINESS_FOR_FILLED_GREAT_WORK_SLOT, BUILDING_FOOD_BONUS_IF_NO_CITIES_AROUND, BUILDING_LOCAL_CITY_CONNECTION_TRADE_ROUTE_MODIFIER, BUILDING_NON_AIR_UNIT_MAX_HEAL)
// 1005: v11.1  (adds BUILDING_DOUBLE_PANTHEON)
# define BUMP_SAVE_VERSION_CITY 1005
// 1000: v7.0   (initial)
# define BUMP_SAVE_VERSION_ESPIONAGE 1000
// 1000: v7.0   (initial)
// 1001: v7.3b  (adds replay events)
// 1002: v11.1a (adds m_uiTotalTimeElapsed)
# define BUMP_SAVE_VERSION_GAME 1002
// 1000: v7.0   (initial)
// 1001: v7.3   (adds Workers Bully Turn)
// 1002: v10.2  (adds ENHANCED_GRAPHS)
# define BUMP_SAVE_VERSION_MINORAI 1002
// 1000: v7.0   (initial)
// 1001: v7.2   (adds ENHANCED_GRAPHS)
// 1002: v7.2a  (adds maya boost GP counters)
// 1003: v8.1b  (adds second bunch of ENHANCED_GRAPHS)
// 1004: v9.0   (adds GODDESS_LOVE_FREE_WORKER, TAJ_MAHAL_STARTS_GA_NEXT_TURN)
// 1005: v10.0  (adds POLICY_FREE_DEFENSIVE_BUILDINGS, GOD_SEA_FREE_WORK_BOAT, UNDERGROUND_SECT_REWORK)
// 1006: v10.0g (adds PENALTY_FOR_DELAYING_POLICIES)
// 1007: v10.2  (adds third bunch of ENHANCED_GRAPHS)
// 1008: v10.2b (adds third bunch of CS_ALLYING_WAR_RESCTRICTION, AUI_GAME_AUTOPAUSE_ON_ACTIVE_DISCONNECT_IF_NOT_SEQUENTIAL)
// 1009: v10.4a (separating CS_ALLYING_WAR_RESCTRICTION for different minors)
// 1010: v10.5  (adds POLICY_MAX_EXTRA_VOTES_FROM_MINORS, POLICY_EXTRA_VOTES, POLICY_DO_TECH_FROM_CITY_CONQ, POLICY_GOLDEN_AGE_YIELD_MOD, POLICY_BUILDINGS_SPECIALIST_COUNT_CHANGE, POLICY_NO_CULTURE_SPECIALIST_FOOD, POLICY_MINORS_GIFT_UNITS, POLICY_PLOT_EXTRA_YIELD_FROM_TRADE_ROUTES, POLICY_NO_CARGO_PILLAGE, POLICY_GREAT_WORK_HAPPINESS, POLICY_SCIENCE_PER_X_FOLLOWERS, POLICY_NO_DIFFERENT_IDEOLOGIES_TOURISM_MOD, POLICY_GLOBAL_POP_CHANGE, POLICY_GREAT_WORK_TOURISM_CHANGES)
// 1011: v10.5c (adds third bunch of POLICY_CITY_SCIENCE_SQUARED_MOD_PER_X_POP)
// 1012: v11.0  (adds BUILDING_YIELD_FOR_EACH_BUILDING_IN_EMPIRE, POLICY_ALLOWS_GP_BUYS_FOR_GOLD)
# define BUMP_SAVE_VERSION_PLAYER 1012
// 1000: v7.0   (initial)
# define BUMP_SAVE_VERSION_POLICIES 1000
// 1000: v7.0   (initial)
# define BUMP_SAVE_VERSION_TEAM 1001
// 1001: v10.x  (adds TECH_ALLOWS_NUKING)
// 1000: v7.0   (initial)
# define BUMP_SAVE_VERSION_TECHS 1000
// 1000: v7.0   (initial)
// 1001: v7.0   (adds TRAIT_ALWAYS_ALLOWED_INNER_TRADE_ROUTES, TRAIT_GOLD_FOR_LUXURY_EXPORT)
# define BUMP_SAVE_VERSION_TRAITS 1001
// 1000: v7.0   (initial)
// 1001: v8.0   (adds NEW_WRITERS_CULTURE_BOMB)
// 1002: v11.0  (adds PROMOTION_NO_UNHAPPINESS_PENALTY)
# define BUMP_SAVE_VERSION_UNIT 1002
// 1000: v7.1   (starts versioning, adds iCreationTurn) -- incompatible with 7.0 (vanilla is ok though)
# define BUMP_SAVE_VERSION_MP_VOTING_SYSTEM 1000
// 1000: v7.2   (initial, adds m_iTimeMilliseconds)
// 1001: v7.3b  (adds m_iExtraData1, m_iExtraData2)
# define BUMP_SAVE_VERSION_REPLAYMESSAGE 1001
// 1000: v7.3b  (initial)
# define BUMP_SAVE_VERSION_REPLAYEVENT 1000
// 1000: v8.2   (initial)
# define BUMP_SAVE_VERSION_RESOLUTIONEFFECTS 1000
// 1000: v11.0  (initial)
# define BUMP_SAVE_VERSION_TREASURY 1000
// 1000: v11.0x (initial)
# define BUMP_SAVE_VERSION_VOTING_CLASSES 1000
#endif
///
#define ENHANCED_GRAPHS
///
#ifdef ENHANCED_GRAPHS
/// First Bunch of Enhanced Graphs
///
#define EG_REPLAYDATASET_FAITHPERTURN
#define EG_REPLAYDATASET_TOTALFAITH
///
#define EG_REPLAYDATASET_NUMOFBORNSCIENTISTS
#define EG_REPLAYDATASET_NUMOFBOUGHTSCIENTISTS
#define EG_REPLAYDATASET_TOTALNUMOFSCIENTISTS
///
#define EG_REPLAYDATASET_NUMOFBORNENGINEERS
#define EG_REPLAYDATASET_NUMOFBOUGHTENGINEERS
#define EG_REPLAYDATASET_TOTALNUMOFENGINEERS
///
#define EG_REPLAYDATASET_NUMOFBORNMERCHANTS
#define EG_REPLAYDATASET_NUMOFBOUGHTMERCHANTS
#define EG_REPLAYDATASET_TOTALNUMOFMERCHANTS
///
#define EG_REPLAYDATASET_NUMOFBORNWRITERS
#define EG_REPLAYDATASET_NUMOFBOUGHTWRITERS
#define EG_REPLAYDATASET_TOTALNUMOFWRITERS
///
#define EG_REPLAYDATASET_NUMOFBORNARTISTS
#define EG_REPLAYDATASET_NUMOFBOUGHTARTISTS
#define EG_REPLAYDATASET_TOTALNUMOFARTISTS
///
#define EG_REPLAYDATASET_NUMOFBORNMUSICIANS
#define EG_REPLAYDATASET_NUMOFBOUGHTMUSICIANS
#define EG_REPLAYDATASET_TOTALNUMOFMUSICIANS
///
#define EG_REPLAYDATASET_NUMOFBORNGENERALS
#define EG_REPLAYDATASET_NUMOFBOUGHTGENERALS
#define EG_REPLAYDATASET_TOTALNUMOFGENERALS
///
#define EG_REPLAYDATASET_NUMOFBORNADMIRALS
#define EG_REPLAYDATASET_NUMOFBOUGHTADMIRALS
#define EG_REPLAYDATASET_TOTALNUMOFADMIRALS
///
#define EG_REPLAYDATASET_NUMOFBOUGHTPROPHETS
#define EG_REPLAYDATASET_TOTALNUMOFPROPHETS
///
#define EG_REPLAYDATASET_GOLDFROMBULLYING
#define EG_REPLAYDATASET_WORKERSFROMBULLYING
///
#define EG_REPLAYDATASET_NUMTRAINEDUNITS
#define EG_REPLAYDATASET_NUMLOSTUNITS
#define EG_REPLAYDATASET_NUMKILLEDUNITS
///
#define EG_REPLAYDATASET_NUMBUILTWONDERS
///
#define EG_REPLAYDATASET_NUMREVEALEDTILES
///
#define EG_REPLAYDATASET_NUMSTOLENSCIENCE
///
#define EG_REPLAYDATASET_DAMAGEDEALTTOUNITS
#define EG_REPLAYDATASET_DAMAGEDEALTTOCITIES
///
#define EG_REPLAYDATASET_DAMAGETAKENBYUNITS
#define EG_REPLAYDATASET_DAMAGETAKENBYCITIES
///
#define EG_REPLAYDATASET_NUMDELEGATES
///
#define EG_REPLAYDATASET_TOTALCHOPS
///
#define EG_REPLAYDATASET_LOSTHAMMERSFROMLOSTWONDERS
///
#define EG_REPLAYDATASET_NUMTIMESOPENEDDEMOGRAPHICS
///
/// Second Bunch of Enhanced Graphs
///
#define EG_REPLAYDATASET_SCIENTISTSTOTALSCIENCEBOOST
#define EG_REPLAYDATASET_ENGINEERSTOTALHURRYBOOST
#define EG_REPLAYDATASET_MERCHANTSTOTALTRADEBOOST
#define EG_REPLAYDATASET_WRITERSTOTALCULTUREBOOST
#define EG_REPLAYDATASET_MUSICIANSTOTALTOURISMBOOST
///
#define EG_REPLAYDATASET_POPULATIONLOSTFROMNUKES
///
#define EG_REPLAYDATASET_CSQUESTSCOMPLETED
#define EG_REPLAYDATASET_ALLIEDCS
///
#define EG_REPLAYDATASET_TIMESENTEREDCITYSCREEN
///
#define EG_REPLAYDATASET_HAPPINESSFROMTRADEDEALS
///
#define EG_REPLAYDATASET_PERCENTOFCITIESWITHACTIVEWLTKD
///
#define EG_REPLAYDATASET_FOLLOWERSOFPLAYERRELIGION
#define EG_REPLAYDATASET_CITIESCONVERTEDTOPLAYERRELIGION
///
#define EG_REPLAYDATASET_TOTALSPECIALISTCITIZENS
#define EG_REPLAYDATASET_PERCENTSPECIALISTCITIZENS // TODO: Fix aesthetics
///
///
/// 
#define EG_REPLAYDATASET_EFFECTIVESCIENCEPERTURN
///
/// Third Bunch of Enhanced Graphs
///
#define EG_REPLAYDATASET_DIEDSPIES
#define EG_REPLAYDATASET_KILLEDSPIES
///
#define EG_REPLAYDATASET_FOODFROMCS
#define EG_REPLAYDATASET_PRODUCTIONFROMCS
#define EG_REPLAYDATASET_CULTUREFROMCS
#define EG_REPLAYDATASET_SCIENCEFROMCS
#define EG_REPLAYDATASET_FAITHFROMCS
#define EG_REPLAYDATASET_HAPPINESSFROMCS
#define EG_REPLAYDATASET_UNITSFROMCS
///
#define EG_REPLAYDATASET_TOURISMPERTURN
#define EG_REPLAYDATASET_NUMGREATWORKSANDARTIFACTS
///
#define EG_REPLAYDATASET_NUMLUXURY
///
#define EG_REPLAYDATASET_NUMWORLDWONDERS
#define EG_REPLAYDATASET_NUMCREATEDWORLDWONDERS
///
#define EG_REPLAYDATASET_NUMGPIMPROVEMENT
///
#define EG_REPLAYDATASET_NUMGOLDONBUILDINGBUYS
#define EG_REPLAYDATASET_NUMGOLDONUNITBUYS
#define EG_REPLAYDATASET_NUMGOLDONUPGRADES
#endif
///
#define GRAPHS_REAL_TOTAL_CULTURE
///
#define GRAPHS_REAL_MILITARY_MIGHT
///
// A more flexible alternative to Replay Messages; primarily for statistics purposes
#define REPLAY_EVENTS

// Adds timestamp for replay messages, saves chat messages
#define REPLAY_MESSAGE_EXTENDED
// Extended replay messages: pseudo-enums
#define REPLAY_MESSAGE_CHAT 7
// IMPORTANT
// Must be defined only ONCE and account for all ReplayMessageTypes additions
#define NUM_REPLAY_MESSAGE_TYPES 8
///
// #define DEV_RECORDING_STATISTICS
// use PreGame.GetNickName with encoded value to retrieve user's SteamId
#define PREGAMEAPI_GET_NETID
///
#define SHOW_ACTUAL_POPULATION
/// Adds UI Drafts screen for staging room -- each player bans X civs and then picks from random set of Y civs
#define INGAME_CIV_DRAFTER
// #define EMERGENCY_LOGGING
/*OTHER CHANGES END*/



/*DUEL MODE STUFF START*/
///
#define DUEL_MOVING_SOME_OPTIONS_TO_DUEL_MODE
///
#define DUEL_BLOCK_BATTLESHIP_RANGE_PROMOTION_ON_DUEL_SIZED_WVE
///
#define DUEL_DOUBLE_UNIT_FAITH_COST
///
#define DUEL_ALWAYS_WAR_ONLY_WITH_MAJORS
///
#define DUEL_SACRED_WATERS_CHANGE
///
#define DUEL_GODDESS_LOVE_CHANGE
///
#define DUEL_GODDESS_STRATEGY_CHANGE
///
#define DUEL_HONOR_CHANGE
///
#define DUEL_GREAT_LIBRARY_CHANGE
///
#define DUEL_DISABLE_GREAT_WALL
///
#define DUEL_TOGGLE_OXFORD_UNIVERSITY
///
#define DUEL_BAN_WORLD_WONDERS
///
#define DUEL_BAN_PANTHEONS
///
#define DUEL_BAN_RELIGION_BELIEFS
///
#define DUEL_DISABLE_MOVE_AFTER_PURCHASE
///
#define DUEL_CANT_CAPTURE_CS_WORKER
///
#define DUEL_TOGGLEABLE_LESS_ALREADY_KNOWN_TECH_COST
///
#define DUEL_WORLD_WONDERS_SAME_TURN
///
#define DUEL_HALICARNASSUS_GP_EXPENDED_GOLD_SCALE
///
#define DUEL_NO_DISBAND
///
#define DUEL_NO_RANDOM_PROPHET
///
#define DUEL_DATABASE
///
#define DUEL_ALLOW_SAMETURN_BELIEFS
///
#define DUEL_ALLOW_GIFT_GP
/*DUEL MODE STUFF END*/

/*TOURNAMENT MODE STUFF START*/
/// IRR/CC/SCRAP proposal conditional restrictions for tournament
#define TOURNAMENT_VOTING_SYSTEM_CHANGES
/*TOURNAMENT MODE STUFF END*/


/*LOGGING START*/
///
#define SLOG(sFmt, ...) {  \
  CvString sRef;  \
  CvString::format(sRef, "[%s:%d]: ", __FUNCTION__, __LINE__);  \
  CvString sMsg;  \
  CvString::format(sMsg, sFmt, __VA_ARGS__);  \
  sRef+= sMsg; \
  LOGFILEMGR.GetLog("PATCH.log", FILogFile::kDontTimeStamp)->Msg(sRef.c_str());  \
}
/*LOGGING END*/

#endif




/// Fixed the free experience recomputation function so now wonders that give free experience globally (as opposed to just units built in one city) work properly
// #define AUI_PLAYER_FIX_RECOMPUTE_FREE_EXPERIENCE_GLOBAL_FREE_EXPERIENCE

/// Fixed base research costs for multi-player teams
// #define AUI_TECH_FIX_TEAMER_RESEARCH_COSTS
/// Fixed research costs for multi-player teams so that they scale with total team city count, and player-based cost modifiers apply only to the effect their cities have on the total modifier
// #define AUI_TECH_FIX_TEAMER_RESEARCH_MODIFIER

/// Restores the malus to coup chance if an enemy spy from the CS ally is present in the CS
// #define AUI_ESPIONAGE_FIX_RESTORE_ENEMY_SPY_COUP_MALUS

/*/// Relocates all per-city and capitol-based yield changes from CvPlot to CvCity, which means that hundredths will be properly accounted for instead of being rounded down immediately
#define AUI_PLOT_FIX_CITY_YIELD_CHANGE_RELOCATED
/// Fixes air sweeping against ground interceptors to show up correctly and properly deal damage to the air unit
#define AUI_UNIT_COMBAT_FIX_AIR_SWEEP_VS_GROUND_INTERCEPTOR
/// The "force end turn" control now checks to make sure nothing (invalid) is blocking it
#define AUI_GAME_FIX_CONTROL_FORCE_END_TURN_CHECKS_FOR_BLOCKING
/// Promotion testing is now done every time a unit's XP is changed, instead of having it manually called all the time
#define AUI_UNIT_TEST_PROMOTION_READY_MOVED

/// If a friendly unit is closer to a blockaded tile than the closest enemy unit, then a tile becomes unblockaded.
#define AUI_CITY_CITIZENS_COUNTERBLOCKADE

/// Citadels can only be captured with a culture bomb if they would have no non-citadel, non-city tiles of friendly culture adjacent to them after the culture bomb
#define AUI_UNIT_CITADEL_RESISTANT_TO_CULTURE_BOMB

// Turn timer stuff

/// Relaxed the limits on the slice limits before turn timers are forced to expire in network multiplayer games (should help stop resyncs due to last-second moves)
#define NQM_GAME_MORE_RELAXED_TURN_SLICE_LIMIT_FOR_NETWORK_MULTIPLAYER*/

// Observer mode fixes
/// Observers will see all resources
#define AUI_PLOT_OBSERVER_SEE_ALL_RESOURCES
/// Observers will have constant visibility over all plots
#define AUI_PLOT_OBSERVER_SEE_ALL_PLOTS
/// Observers are set to have met every team in the game
#define AUI_GAME_OBSERVER_MEET_ALL_TEAMS
/// Natural wonder pop-ups do not trigger for observers
#define AUI_PLOT_OBSERVER_NO_NW_POPUPS
/// Observers can now open the city screen as if they had spies in every city
// #define AUI_GAME_OBSERVER_CAN_OPEN_CITIES
/// All cities are set to be revealed to observers
// #define AUI_CITY_OBSERVER_REVEALS_ALL_CITIES

/*// Altered score calculations as an option toggleable in-game
#define NQM_OPTIONAL_SCORING_TWEAKS

// Fixes specific to simultaneous mode
/// Units can no longer insta-heal twice in the same turn. This should only affect simultaneous mode and stops a few cheesey strategies
#define NQM_UNIT_FIX_NO_DOUBLE_INSTAHEAL_ON_SAME_TURN*/
/*/// Paradroping a unit disqualifies it from insta-healing in the same turn. This should only affect simultaneous mode and stops a few cheesey strategies
#define NQM_UNIT_FIX_NO_INSTAHEAL_AFTER_PARADROP
/// Fortifying a unit will only give it a strength boost at the end of the turn. This should only affect simultaneous mode and stops players from fortifying units at the start of the turn for the defensive bonus, then using those units as normal.
#define NQM_UNIT_FIX_FORTIFY_BONUS_RECEIVED_END_OF_TURN_NOT_INSTANTLY
/// If the game/player who is in simultaneous mode declares war and the turn timer has less than 60 seconds remaining, it is filled back up to 60 seconds
//#define NQM_GAME_EXTEND_TURN_TIMER_ON_LAST_MINUTE_WAR_DECLARATION_IF_SIMULTANEOUS*/

/*/// Anti-air ground/sea units can no longer intercept planes after they move until the player ends their turn. This should only affect simultaneous mode and stops players from deliberately moving in AA after another player has done air sweeps.
#define NQM_UNIT_NO_AA_INTERCEPT_AFTER_MOVE_BEFORE_TURN_END
/// Fighters set to intercept duty can only perform interceptions after the player ends their turn. This should only affect simultaneous mode and stops players from turning on intercepts mid-turn to make air sweeps unreliable.
#define NQM_UNIT_FIGHTER_NO_INTERCEPT_UNTIL_AFTER_TURN_END
// New hybrid mode
/// When in hybrid mode, players who are not at war with each other have their turns happen simultaneously, thus speeding games up significantly
#define AUI_GAME_BETTER_HYBRID_MODE
#ifdef AUI_GAME_BETTER_HYBRID_MODE
/// Turn lengths are determined for each player at the beginning of the turn and cached, instead of having turn lengths be a global variable whose value can change mid-turn
#define AUI_GAME_PLAYER_BASED_TURN_LENGTH
#endif

// Pathfinder (A*) optimizations, tweaks, and fixes
/// Removes instances of alloc, malloc, and firemalloc from AStar and replaces them with new (also replaced corresponding free calls with delete calls)
#define AUI_ASTAR_REMOVE_MALLOC
/// A* functions no longer run the canEnterTerrain() functions during validation (it should normally be run once and cached, but Firaxis did a bunch of stupids)
#define AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
/// Moves the check for whether a node has no parent to the beginning of PathValid() (originally from Community Patch)
#define AUI_ASTAR_FIX_PARENT_NODE_ALWAYS_VALID_OPTIMIZATION
/// Reorders some checks to make sure ones that process faster get executed first (if they fail, then the function skips checking the slower ones)
#define AUI_ASTAR_FIX_FASTER_CHECKS
/// Calculates the neighbors of a tile on pathfinder initialization instead of at path construction (originally from Community Patch)
#define AUI_ASTAR_PRECALCULATE_NEIGHBORS_ON_INITIALIZE
/// Minor A* optimizations
#define AUI_ASTAR_MINOR_OPTIMIZATION
/// Pathfinders now have a built-in turn limiter that invalidates too long paths they are being built instead of after the best path has been calculated (also enables calculating turns to a target from a plot other than the unit's current plot)
#define AUI_ASTAR_TURN_LIMITER
/// Gets the last node before the parent (used for planning melee attacks to know where they'd attack from)
#define AUI_ASTAR_GET_PENULTIMATE_NODE
/// Fixes possible null pointer dereferences in A*
#define AUI_ASTAR_FIX_POSSIBLE_NULL_POINTERS
/// Human-controlled missionaries and units will still want to avoid undesirable tiles a bit when planning paths, though not to the full extent that an AI-controlled unit would (parameter value is the extra "cost" weight added)
#define AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT (1)
/// Pointers to the plot representing each A* node are stored in the A* node in question
#define AUI_ASTAR_CACHE_PLOTS_AT_NODES
/// Adds a new function that is a middle-of-the-road fix for allowing the functions to take account of roads and railroads without calling pathfinder too often
#define AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
/// The danger of a tile will only be considered when checking path nodes, not when checking the destination (stops units from freezing in panic)
#define AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_PATH
/// The path's destination's danger value will be considered instead of the original plot's danger value, otherwise we're just immobilizing AI units (oddly enough, the Civ4 algorithm is fine, only the Civ5 ones needed to be fixed)
#define AUI_ASTAR_FIX_CONSIDER_DANGER_USES_TO_PLOT_NOT_FROM_PLOT
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_TO_PLOT_NOT_FROM_PLOT
/// If the pathfinder does not ignore danger, the plot we're moving from must pass the danger check before we consider the destination plot's danger
#define AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_POSITIVE_DANGER_DELTA
#endif
/// If the pathfinder does not ignore danger, use the unit's combat strength times this value as the danger limit instead of 0 (important for combat units)
#define AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH (6)
/// Mountain tiles are no longer automatically marked as invalid steps
#define AUI_ASTAR_FIX_STEP_VALID_CONSIDERS_MOUNTAINS
#ifdef AUI_PLOT_GET_VISIBLE_ENEMY_DEFENDER_TO_UNIT
/// When a unit will attack onto a plot, it will try to minimize the damage it would receive from each plot candidate. If it will always die, it will instead maximize dealt damage.
#define AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING
#endif
#ifndef AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING // Already handled by the other algorithm
/// When a unit will attack onto a plot, river crossings are avoided whenever possible
#define AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING
#endif
/// Units without a defense bonus still consider a tile's defense penalties when pathfinding
#define AUI_ASTAR_FIX_DEFENSE_PENALTIES_CONSIDERED_FOR_UNITS_WITHOUT_DEFENSE_BONUS
/// Fixes a nonsense territory check in the Ignore Units pathfinder (can this unit enter territory owned by this unit's team?) by replacing it with one that actually makes sense (can this unit enter territory of the player who owns the target plot?)
#define AUI_ASTAR_FIX_IGNORE_UNITS_PATHFINDER_TERRITORY_CHECK
/// In addition to the movement cost from features on a tile, the route recommender will now also consider the movement cost of moving onto a tile with hills
#define AUI_ASTAR_FIX_BUILD_ROUTE_COST_CONSIDER_HILLS_MOVEMENT
/// When a unit is set to auto-explore, it will consider how many tiles are revealed past the 1st ring as well
#define AUI_ASTAR_FIX_MAXIMIZE_EXPLORE_CONSIDER_2ND_RING_NONREVEALED
/// Units that are on automated explore will prefer to visit tiles that have a higher "see from" stat instead of just hills (eg. helps with Carthage)
#define AUI_ASTAR_FIX_MAXIMIZE_EXPLORE_UNHARDCODE_HILL_PREFERENCE
/// Tweaks the amount of extra cost a tile receives for each new tile it doesn't reveal
#define AUI_ASTAR_TWEAKED_PATH_EXPLORE_NON_REVEAL_WEIGHT (50)
/// Units with the Explore UnitAIType will always move to maximize exploration
#define AUI_ASTAR_EXPLORE_UNITAITYPE_ALWAYS_MAXIMIZES_EXPLORE
/// Trade routes will prefer tiles owned by either party over unowned tiles, and will prefer unowned tiles over tiles owned by a third party
#define AUI_ASTAR_TRADE_ROUTE_COST_TILE_OWNERSHIP_PREFS

// Binomial RNG Stuff (Delnar: the binomial RNG generates numbers in a binomial distribution instead of a flat one like the regular RNG)
/// Enables the Binomial Random Number Generator (originally from Artificial Unintelligence)
#define AUI_BINOM_RNG
#ifdef AUI_BINOM_RNG
/// Generated ridges use the binomial RNG for directional bias strength and ridge weakness
#define AUI_FRACTAL_RIDGE_USE_BINOM_RNG
/// When modifying a personality value (eg. Boldness, Wonder Competitiveness), the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_GET_RANDOM_PERSONALITY_WEIGHT_USES_BINOM_RNG
/// When adding or subtracting flavor value, the binomial RNG is used to generate a normal distribution instead of a flat one
#define AUI_FLAVOR_MANAGER_GET_ADJUSTED_VALUE_USES_BINOM_RNG
/// Adds an option to the game to use the binomial RNG for combat rolls. To compensate for binomial weight, range is increased so that the original range falls between ±3σ (i.e. a 134x range increase)
#define NQM_COMBAT_RNG_USE_BINOM_RNG_OPTION
#endif

// AI fixes that significantly affect MP
/// Disables the active grand strategy for AIs operating behind human players, the primary effect of which is that puppets don't do as much weird stuff
#define AUI_GS_FIX_NO_ACTIVE_GS_FOR_HUMANS
/// Instead of ignoring all military training buildings (eg. stables, kreposts, etc.), puppets will instead nullify the Military Training and Naval flavors
#define AUI_CITYSTRATEGY_FIX_CHOOSE_PRODUCTION_PUPPETS_NULLIFY_BARRACKS
/// Instead of prefering maintenance-free buildings, puppets will now use a logistic scale to emphasize low-maintenance buildings
#define AUI_CITYSTRATEGY_FIX_CHOOSE_PRODUCTION_SLIDING_LOGISTIC_MAINTENANCE_SCALE
/// Puppets can now build non-wonder buildings that they would normally have 0 weight for
#define AUI_CITYSTRATEGY_PUPPETS_ALLOW_BAD_BUILDS_IF_NO_OTHER_CHOICE
/// Scales the GetLastTurnWorkerDisbanded() computation to game speed
#define AUI_CITYSTRATEGY_FIX_TILE_IMPROVERS_LAST_DISBAND_WORKER_TURN_SCALE
/// If a player does not have any non-scouting military units, the "enough workers" city strategy is triggered and the "want workers" and "need workers" city strategies always return false
#define AUI_CITYSTRATEGY_DONT_EMPHASIZE_WORKERS_IF_NO_MILITARY
/// Fixes the "zeroed out flavor" check to still accept 0 as a possible flavor value, but not accept negative values
#define AUI_FLAVOR_MANAGER_FIX_RANDOMIZE_WEIGHTS_ZEROED_OUT_FLAVOR
/// Fixes the function messing up and returning the wrong adjustment when the value to be added is actually negative (eg. for minor civs)
#define AUI_FLAVOR_MANAGER_FIX_GET_ADJUSTED_VALUE_NEGATIVE_PLUSMINUS
/// If the first adjusted value is out of bounds, keep rerolling with the amount with which it is out of bounds until we remain in bounds
#define AUI_FLAVOR_MANAGER_GET_ADJUSTED_VALUE_USE_REROLLS
/// Plots that are under immediate threat by an even number of units will properly return that the plot is under immediate threat
#define AUI_DANGER_PLOTS_FIX_ADD_DANGER_WITHIN_ONE_MOVE
/// The ignore visibility switch for DangerPlots also works on the plot visibility check
#define AUI_DANGER_PLOTS_FIX_SHOULD_IGNORE_UNIT_IGNORE_VISIBILITY_PLOT
/// Minors will always "see" units of major civs in tiles (value) away from their city (since minors don't scout) when plotting danger values (stops excessive worker stealing)
#define AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MINORS_SEE_MAJORS (5)
/// Minors will ignore all units of players who are not at war with them
#define AUI_DANGER_PLOTS_FIX_IS_DANGER_BY_RELATIONSHIP_ZERO_MINORS_IGNORE_ALL_NONWARRED
/// Minors will assume trespassing units are there for war
#define AUI_DANGER_PLOTS_IS_DANGER_BY_RELATIONSHIP_ZERO_MINORS_DO_NOT_IGNORE_TRESSPASSERS
/// Fixes bad code for visible barbarian units adding to "barbarian threat" value (affects CS)
#define AUI_MILITARY_FIX_BARBARIAN_THREAT
/// If the AI's religion now unlocks multiple faith buildings, AI can now purchase all of them
#define AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
/// Fixes the check for whether ranged damage would be more than heal rate to use >= instead of >, adds a flat value to total damage at start (both make up for randomness), and treats cities as an expected damage source instead of a flat "yes"
#define AUI_UNIT_FIX_UNDER_ENEMY_RANGED_ATTACK_HEALRATE (1)
/// The AI will consider promises it made not to convert cities of a player when choosing a prophet conversion target
#define AUI_RELIGION_FIX_CHOOSE_PROPHET_CONVERSION_CITY_HONOR_NONCONVERT_PROMISE

// Flavor system changes (affect CS build queue and puppet build queue)
/// Free buildings and units that a building would generate are factored into the flavor
#define AUI_BUILDING_PRODUCTION_AI_CONSIDER_FREE_STUFF
/// Policies can now alter the flavors of certain buildingclasses (disabled for now because it crashes at initialization)
//#define AUI_POLICY_BUILDING_CLASS_FLAVOR_MODIFIERS
/// Beliefs can now alter the flavors of certain buildingclasses
//#define AUI_BELIEF_BUILDING_CLASS_FLAVOR_MODIFIERS

// Citizen Management Fixes
/// Extra food value assigned to specialists for half food consumption now depends on the XML value for citizen food consumption (instead of assuming the default value)
#define AUI_CITIZENS_FIX_SPECIALIST_VALUE_HALF_FOOD_CONSUMPTION
/// Replaces the rudimentary specialist-plot check with a plot vs. default citizen value check
#define AUI_CITIZENS_IS_PLOT_BETTER_THAN_DEFAULT_SPECIALIST
/// The value of excess food is multiplied by the city's growth modifiers
#define AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
/// If a tile would provide enough food to generate excess food, the excess amount has its value halved as if the city was already generating enough food
#define AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
/// If a city is using excess food for production, change the value of food yields accordingly (eg. city cannot starve, food is not 1:1 with production)
#define AUI_CITIZENS_GET_VALUE_ALTER_FOOD_VALUE_IF_FOOD_PRODUCTION
/// If a city has any modifiers on certain yield incomes, this modification is applied to the value of those yields as well (eg. +25% gold increases gold yield value by 25%)
#define AUI_CITIZENS_GET_VALUE_CONSIDER_YIELD_RATE_MODIFIERS
/// Games where happiness is disabled no longer cause the citizen manager to always ignore the avoid growth checkbox
#define AUI_CITIZENS_FIX_AVOID_GROWTH_FLAG_NOT_IGNORED_IF_NO_HAPPINESS
#ifndef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS // The define below is only useful if growth modifiers aren't considered
/// The citizen manager only forces avoid growth from low happiness when growing in the city would lower the player's happiness. Also, avoid growth is enabled for cities that would grow next turn to make the empire unhappy.
#define AUI_CITIZENS_FIX_FORCED_AVOID_GROWTH_ONLY_WHEN_GROWING_LOWERS_HAPPINESS
#endif
/// When comparing whether a specialist is better than an unemployed citizen, regular value evaluation is used
#define AUI_CITIZENS_IS_BETTER_THAN_DEFAULT_SPECIALIST_USE_REGULAR_VALUES
/// The value of food is now properly increased if the city is on food focus with the avoid growth flag set
#define AUI_CITIZENS_FIX_GET_VALUE_FOOD_YIELD_VALUE_WHEN_STARVATION_WITH_AVOID_GROWTH
/// Instead of cities with 4 or less population and set to either no focus or partial food focus multiplying food value by 4, all cities with 4 or less population will want at least 2 food just like ones set to no focus or partial/full food focus
#define AUI_CITIZENS_LOW_POPULATION_CITIES_USE_2MIN_NOT_4X_FOOD
/// If a city is set to avoid growth, it will assign a value of 1 to excess food instead of 0. While it's still much less than a Gold yield, it at least makes 3 food tiles more preferable to 2 food tiles even when avoiding growth
#define AUI_CITIZENS_AVOID_GROWTH_STILL_VALUES_EXCESS_FOOD
/// Unhardcodes the value assigned to specialists for happiness (flat value is the base multiplier for value of a single happiness point before modifications)
#define AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_HAPPINESS (8)
#ifdef AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_HAPPINESS
/// Citizen Manager considers the effect of half specialist unhappiness on other yields
#define AUI_CITIZENS_CONSIDER_HAPPINESS_VALUE_ON_OTHER_YIELDS
#endif
/// If the player has negative gold income and would lose science as a result, gold yield is scored as if it were science yield
#define AUI_CITIZENS_GOLD_YIELD_COUNTS_AS_SCIENCE_WHEN_IN_DEFICIT
/// The function that removes the worst specialist from their slot actually removes the worst one instead of just the first specialist encountered
#define AUI_CITIZENS_FIX_REMOVE_WORST_SPECIALIST_ACTUALLY_REMOVES_WORST
/// Adds a self-consistency check function to citizen manager, which constantly shifts the worst scoring citizen to the best scoring spot until it's not actually shifting the citizen or it keeps shifting back and forth between the same spots.
#define AUI_CITIZENS_SELF_CONSISTENCY_CHECK (0) // This is the score difference threshold below which the SC loop will terminate
#ifdef AUI_CITIZENS_SELF_CONSISTENCY_CHECK
/// Reallocate citizens runs a self-consistency check after it reallocates everyone
#define AUI_CITIZENS_REALLOCATE_CITIZENS_USES_SELF_CONSISTENCY
#ifdef AUI_YIELDS_APPLIED_AFTER_TURN_NOT_BEFORE
/// After reallocating citizens in all cities at the beginning of the turn, a self-consistency check is run for all cities (so that empire-specific scoring isn't as highly weighted for older cities)
#define AUI_PLAYER_SELF_CONSISTENCY_SWEEP_AFTER_INITIAL_REALLOCATE
#endif
#ifdef AUI_CITIZENS_GET_VALUE_ALTER_FOOD_VALUE_IF_FOOD_PRODUCTION
/// If a city is in food production mode when it reallocates its citizens, it will perform a mock reallocation first assuming no food production, then run a self-consistency check afterwards
#define AUI_CITIZENS_FOOD_PRODUCTION_TRIAL_RUN_THEN_SELF_CONSISTENCY
#endif
/// Citizen (re)assignments mid-turn and mid-turn global yield changes (e.g. to happiness) instantly run a self-consistency check on all cities
#define AUI_CITIZENS_MID_TURN_ASSIGN_RUNS_SELF_CONSISTENCY
#endif
/// If a city's religion has the Guruship belief, the citizen manager will account for the extra production gained from the first citizen slot
#define AUI_CITIZENS_GET_SPECIALIST_VALUE_ACCOUNT_FOR_GURUSHIP
/// Puppets not set to soft or hard avoid growth will still want at least 1 excess food (Gold Focus would disallow this). This removes need for some code that helps avoid starvation but could be abused by a human, so this code was disabled.
#define AUI_CITIZENS_PUPPET_STILL_WANTS_GROWTH
/// Puppeting or annexing a city will immediately reallocate citizens and set focuses when the order is given. It also means players don't have to manually remove Gold focus when unpuppeting a city.
#define AUI_CITIZENS_PUPPET_AND_ANNEX_REALLOCATES_CITIZENS
/// The citizen manager will instantly reallocate citizens if it switches onto or off of food production mode (i.e. the mode where excess food contributes toward production instead of growth)
#define AUI_CITIZENS_REALLOCATE_ON_FOOD_PRODUCTION_CHANGE
/// Changes the citizen manager so that locked tiles that get blocked (through siege or blockade) are not counted as locked until the tile is no longer blocked
#define AUI_CITIZENS_FIX_LOCKED_TILES_BLOCKED

// City Governor Stuff
/// Fixes various possible bugs by replacing std::vector's resize() and [] calls with push_back() and clear()
#define AUI_CITY_FIX_GET_NEXT_BUYABLE_PLOT_VECTOR_USE_RESERVE
/// Replaces the "lower influence cost by 1 if near NW or resource" code with code that lowers the influence cost of plots that are between resources or natural wonders and the closest owned plot of the city
#define AUI_CITY_GET_BUYABLE_PLOT_LIST_RESOURCE_NW_OSMOSIS
/// Weights the yield contribution to a plot's influence cost by the value of the yield to citizen automation.
#define AUI_CITY_GET_BUYABLE_PLOT_LIST_WEIGHTED_YIELDS
#if defined(AUI_CITY_GET_BUYABLE_PLOT_LIST_RESOURCE_NW_OSMOSIS) || defined(AUI_CITY_GET_BUYABLE_PLOT_LIST_WEIGHTED_YIELDS)
/// City focus is considered when the city governor weighs the values of certain yields over others
#define AUI_CITY_GET_BUYABLE_PLOT_LIST_CONSIDER_CITY_FOCUS
#endif
/// Actually makes passive acquisition of tiles not adjacent to an already owned tile impossible
#define AUI_CITY_GET_BUYABLE_PLOT_LIST_ACTUALLY_IMPOSSIBLE_IF_NOT_ADJACENT_OWNED
/// Delnar: Moved Fruitstrike's code to prioritize plots with the lowest gold purchasing cost in the case of ties into the end of GetBuyablePlotList() so that plots will still be randomly decided if their gold purchasing costs are the same (instead of prioritizing Northeastern plots)
#define NQM_CITY_GET_NEXT_BUYABLE_PLOT_MOVE_GOLD_PURCHASE_COST_PRIORITY_TO_GET_BUYABLE_PLOT_LIST

// AI/Automated Worker fixes
/// AI/Automated Inca workers know that there is no maintenance on hills, so routines are adjusted as a result
#define AUI_WORKER_INCA_HILLS
/// AI/Automated workers do not care about the build time or cost of scrubbing fallout
#define AUI_WORKER_FIX_FALLOUT
/// Divides score for improvement if built for a puppeted city
#define AUI_WORKER_SCORE_PLOT_REDUCED_PUPPET_SCORE (2)
/// No longer artificially increases the score for plots owned by the player's capitol or an original capitol city
#define AUI_WORKER_SCORE_PLOT_NO_CAPITOL_FAVORING
/// Multiplies score for improvement if built on a plot that is either already worked or would be worked by the owning city
#define AUI_WORKER_SCORE_PLOT_MULTIPLY_SCORE_IF_WOULD_WORK (2)
/// Returns score of 0 for improvement if built for a city being razed
#define AUI_WORKER_SCORE_PLOT_NO_SCORE_FROM_RAZE
/// If building an improvement also generates flat hammers, consider the effect as flat +parameter hammer yield
#define AUI_WORKER_SCORE_PLOT_CHOP (0.5)
/// Removes the bias to chop forests after optics (since it doesn't actually offer a gameplay improvement)
#define AUI_WORKER_NO_CHOP_BIAS
/// Faith now affects tile evaluation for workers, it pulls from culture multiplier though
#define AUI_WORKER_EVALUATE_FAITH
/// AI/Automated workers value strategic resources that a player has none of higher than strategic resources that the player has used all of
#define AUI_WORKER_TWEAKED_DONT_HAVE_MULTIPLIER (6)
/// Combat workers will increase the maximum allowed plot danger value to their current strength times this value
#define AUI_WORKER_SHOULD_BUILDER_CONSIDER_PLOT_MAXIMUM_DANGER_BASED_ON_UNIT_STRENGTH (6)
/// FindTurnsAway() no longer returns raw distance, parameter dictates whether we're reusing paths and ignoring units (fast but rough) or not (slow but accurate)
#define AUI_WORKER_FIND_TURNS_AWAY_USES_PATHFINDER (true)
#ifdef AUI_PLOT_CALCULATE_STRATEGIC_VALUE
/// AddImprovingPlotsDirective() now processes improvement defense rate
#define AUI_WORKER_ADD_IMPROVING_PLOTS_DIRECTIVE_DEFENSIVES
#endif
/// Shifts the check for whether there already is someone building something on the plot to the necessary AddDirectives() functions (so collaborative building is possible)
#define AUI_WORKER_FIX_SHOULD_BUILDER_CONSIDER_PLOT_EXISTING_BUILD_MISSIONS_SHIFT
/// New function that is called by AI/Automated workers to construct non-road improvements in a minor's territory (eg. for Portugal)
#define AUI_WORKER_ADD_IMPROVING_MINOR_PLOTS_DIRECTIVES
/// Multiplies the weight of unowned luxury resources for plot directives depending on the empire's happiness (value is the multiplier at 0 happiness)
#define AUI_WORKER_GET_RESOURCE_WEIGHT_INCREASE_UNOWNED_LUXURY_WEIGHT (2.0)
/// Consider extra sources of happiness once a resource is obtained (eg. extra happiness from luxury resources via policy, extra happiness from resource variety)
#define AUI_WORKER_GET_RESOURCE_WEIGHT_CONSIDER_EXTRAS_FOR_HAPPINESS_FROM_RESOURCE
/// Removes the isAdjacent check for whether a work boat can access an area different from its current one (pathfinder takes care of bad cases anyway, it's just a bit slower)
#define AUI_WORKER_FIX_SHOULD_CONSIDER_PLOT_WORK_BOATS_CONSIDER_ALL_SEA_PLOTS
/// Only disregard an impassable plot if the unit cannot enter impassable plots
#define AUI_WORKER_FIX_SHOULD_CONSIDER_PLOT_FLYING_WORKER_DISREGARDS_PEAKS
/// Added some extra checks for Celts so that 1) they will improve forests when there would still be enough unimproved ones remaining to give the same faith bonus and 2) they will not improve luxury resources on forests if they do not get any use out of them and would lower faith
#define AUI_WORKER_FIX_CELTIC_IMPROVE_UNIMPROVED_FORESTS
/// AI/Automated workers will no longer automatically continue building the improvement they are currently building if the tile they are on is in danger (instead of having this behavior trigger in CvHomelandAI)
#define AUI_WORKER_EVALUATE_WORKER_RETREAT_AND_BUILD
/// AI/Automated workers will now consider any modifiers the player has to road maintenance when calculating how much profit the road earns
#define AUI_WORKER_FIX_CONNECT_CITIES_TO_CAPITOL_CONSIDER_MAINTENANCE_MODIFIERS
/// No longer requires that an improvement enable use of a bonus resource, since the projected plot yields will be higher from unlocking the resource anyway
#define AUI_WORKER_FIX_IMPROVING_PLOTS_DIRECTIVE_DONT_REQUIRE_BONUS_RESOURCE_UNLOCKER
#ifdef AUI_PLAYER_CACHE_UNIQUE_IMPROVEMENTS
/// Unhardcodes the fact that the AI will not remove features that are needed to construct a civ's unique improvement
#define AUI_WORKER_UNHARDCODE_NO_REMOVE_FEATURE_THAT_IS_REQUIRED_FOR_UNIQUE_IMPROVEMENT
#endif

// HomelandAI fixes; used by automated workers/scouts and extensively by CS
/// Disables the code that would start fortifying scouts if recon state was set as "enough"
#define AUI_HOMELAND_ALWAYS_MOVE_SCOUTS
/// Tweaks the algorithm for Plot Heal Moves to keep March promotions in mind and make sure we don't overheal if we're under threat
#define AUI_HOMELAND_TWEAKED_HEAL_MOVES
/// Changes the AcceptableDanger value in PlotDangerMoves to be a function of the unit's current HP percent
#define AUI_HOMELAND_TWEAKED_ACCEPTABLE_DANGER (1.0)
/// When finding patrol targets for civilian units, subtract off danger value from plot score
#define AUI_HOMELAND_TWEAKED_FIND_PATROL_TARGET_CIVILIAN_NO_DANGER
/// Border plots, plots containing routes, and plots containing civilians are preferred over others when patrolling
#define AUI_HOMELAND_FIND_PATROL_TARGET_DESIRES_BORDER_AND_ROUTE_AND_CIVILIAN_GUARD
/// Disbanding explorers now uses the scrap() function instead of the kill() function
#define AUI_HOMELAND_FIX_EXECUTE_EXPLORER_MOVES_DISBAND
/// Stops the AI from suiciding units by embarking them onto tiles that can be attacked
#define AUI_HOMELAND_FIX_EXECUTE_MOVES_TO_SAFEST_PLOT_NO_EMBARK_SUICIDE
/// If an AI/automated worker can still move after it has reached its target, allow it to queue up a build order
#define AUI_HOMELAND_FIX_EXECUTE_WORKER_MOVE_MOVE_AND_BUILD
/// AI/Automated explorers now move after popping goody huts
#define AUI_HOMELAND_FIX_EXECUTE_EXPLORER_MOVES_MOVE_AFTER_GOODY
/// Uses the unit's in-game movement range for plot search heuristic instead of relying on the unit's info's pre-determined movement range
#define AUI_HOMELAND_FIX_EXECUTE_MOVES_TO_SAFEST_PLOT_USE_GAME_MOVEMENT_RANGE
/// Civilian units execute moves to safety instead of patrolling
#define AUI_HOMELAND_FIND_PATROL_MOVES_CIVILIANS_PATROL_TO_SAFETY
/// After moving workers around, the AI will move combat units around to guard those workers. Not ideal because AI will still not send workers to dangerous tiles even if they'd have a protector there, but better than nothing.
#define AUI_HOMELAND_PLOT_WORKER_MOVES_ALSO_PLOTS_WORKER_DEFENSE

// Voting/League AI Stuff for when a player is defeated but their AI can still vote on proposals
/// When voting for a player, the AI will now adjust for the fact that the voting system is First-Past-The-Post (so it will try to vote against players as well)
#define AUI_VOTING_SCORE_VOTING_CHOICE_PLAYER_ADJUST_FOR_FPTP
/// Uses a different algorithm for scoring voting on world ideology
#define AUI_VOTING_TWEAKED_WORLD_IDEOLOGY
/// Uses a different algorithm for scoring voting on world religion
#define AUI_VOTING_TWEAKED_WORLD_RELIGION
/// Alters some of the algorithms and values used for scoring a proposal overall (instead of just its effects), primarily based on diplomatic relations with the proposer
#define AUI_VOTING_TWEAKED_PROPOSAL_SCORING

// Start Positioner Fixes (for WorldBuilder Maps and maps not randomly generated but with random starting locations)
/// Civilizations that are marked as coastal get the same coastal bias as maritime city-states
#define AUI_STARTPOSITIONER_COASTAL_CIV_WATER_BIAS
/// When calculating the founding value of a tile, tailor the SiteEvaluation function to the current player instead of the first one
#define AUI_STARTPOSITIONER_FLAVORED_STARTS

// Weird stuff
/// Adds ranged counterattacks to the game, toggleable for now via in-game option
#define DEL_RANGED_COUNTERATTACKS


// GlobalDefines (GD) wrappers
// INT
#define GD_INT_DECL(name)       int m_i##name
#define GD_INT_DEF(name)        inline int get##name() { return m_i##name; }
#define GD_INT_INIT(name, def)  m_i##name(def)
#define GD_INT_CACHE(name)      m_i##name = getDefineINT(#name)
#define GD_INT_GET(name)        GC.get##name()
// FLOAT
#define GD_FLOAT_DECL(name)       float m_f##name
#define GD_FLOAT_DEF(name)        inline float get##name() { return m_f##name; }
#define GD_FLOAT_INIT(name, def)  m_f##name(def)
#define GD_FLOAT_CACHE(name)      m_f##name = getDefineFLOAT(#name)
#define GD_FLOAT_GET(name)        GC.get##name()
// DOUBLE (high precision, but much slower than float)
#ifdef NQM_CACHE_DOUBLE
#define GD_DOUBLE_DECL(name)       double m_d##name
#define GD_DOUBLE_DEF(name)        inline double get##name() { return m_d##name; }
#define GD_DOUBLE_INIT(name, def)  m_d##name(def)
#define GD_DOUBLE_CACHE(name)      m_d##name = getDefineDOUBLE(#name)
#define GD_DOUBLE_GET(name)        GC.get##name()
#endif

// --- NQ Changes ---
// Always show barb camps to all players instead of locking it to a policy
#define NQ_ALWAYS_SEE_BARB_CAMPS
// Allow hill terrain changes based on buildings (Swedish UB: Falu Gruva)
#define NQ_ALLOW_BUILDING_HILL_YIELD_CHANGES
// Can no longer purchase Great Scientists with Faith
#define NQ_NO_FAITH_PURCHASING_SCIENTISTS
// Game Option: Extra Religions (allows any number of religions up to max players or 8, whichever is lower)
#define NQ_ALLOW_EXTRA_RELIGIONS
// Sweden UA now gives flat science in the empire when a great person is born
#define NQ_SCIENCE_PER_GREAT_PERSON_BORN
// Spoils of War - old Assyria UA now applies to all players by default
#define NQ_SPOILS_OF_WAR
// Treasures of Nineveh now gives a free Great Work the first time each city is conquered
#define NQ_GREAT_WORK_ON_UNIQUE_CONQUEST
// City States and Barbarian units no longer give any Great General points
#define NQ_NO_GG_POINTS_FROM_CS_OR_BARBS
// FasterAlongRiver now requires the plot you are coming from to be a river plot as well
#define NQ_FIX_FASTER_ALONG_RIVER
// Add HeavyChargeDownhill promotion for Carthage Elephant
#define NQ_HEAVY_CHARGE_DOWNHILL
// China's new UA gives bonus movement when turn starts on or adjacent to great general
#define NQ_ART_OF_WAR_PROMOTION
// Civ-specific luxuries (Indonesia) disallowed from being required for City State quests an WLtKD requests
#define NQ_NO_UNIQUE_LUX_REQUESTS
// extra missionary spreads from religious belief
#define NQ_BELIEF_EXTRA_MISSIONARY_SPREADS

// allow free settlers from beliefs
#define NQ_FREE_SETTLERS_FROM_BELIEF
// plot culture cost modifier from beliefs now only impacts religioun founder's cities
#define NQ_PLOT_COST_FROM_BELIEF_IS_UNIQUE
// free units from belief of Deus Vult
#define NQ_DEUS_VULT

// missionary cost reduction now only affects founder of religion and no longer applies to inquisitors
#define NQ_FIX_MISSIONARY_COST_MODIFIER_BELIEF
// faith costs now rounded to 5 instead of 10
#define NQ_FAITH_COST_ROUNDS_TO_NEAREST_5
// faith per foreign trade route
#define NQ_FAITH_PER_FOREIGN_TRADE_ROUTE
// religious pressure modifier to your cities vs cities that aren't yours
#define NQ_SPREAD_MODIFIER_OWNED_CITIES
// faith per turn for city states that follow this religion
#define NQ_FAITH_PER_CITY_STATE_THIS_RELIGION
// cheating so that sacred sites applies both tourism and gold
#define NQ_CHEAT_SACRED_SITES_AFFECTS_GOLD
// all players can rush spaceship parts with Engineers now
//#define NQ_ALLOW_SS_PART_HURRY_BY_DEFAULT // --- disabled
// fix tourism from futurism so it scales at different game speeds
#define NQ_FIX_ADD_TOURISM_GAME_SPEED_MOD
// golden age points accrue even while in golden ages
#define NQ_GOLDEN_AGE_OVERHAUL
// show base influence you would have at peace in tooltip while at war with city states
#define NQ_SHOW_BASE_INFLUENCE_WHILE_AT_WAR_IN_CS_TOOLTIP
// disable gifting of great people between major powers
#define NQ_NO_GIFTING_GREAT_PEOPLE_TO_MAJORS
// world's fair # of turns scales with game speed now
#define NQ_WORLDS_FAIR_GAME_SPEED_SCALING
// one shot local population change when a building is constructed
#define NQ_LOCAL_POPULATION_CHANGE_FROM_BUILDING
// Lua hook for game event when reformation belief is added
#define NQ_ADD_REFORMATION_LUA_HOOK
// Food bonus for removing features
#define NQ_FOOD_FROM_CHOPS
// Unit turn ends on final attack even with multiple attacks per turn
#define NQ_UNIT_TURN_ENDS_ON_FINAL_ATTACK*/
// units that cross rivers always lose all movement unless amphibious (broken by 3/2 movement remaining bug)
/////#define NQ_FIX_MOVES_THAT_CONSUME_ALL_MOVEMENT
/*// clearing barbarian camps gives influence to all nearby city states
// spawning prohets now reduces faith by the amount required instead of all faith
#define NQ_SPAWN_PROPHETS_REMOVE_ONLY_REQUIRED_FAITH
// flat faith whenever a citizen is born from beliefs
#define NQ_FLAT_FAITH_PER_CITIZEN_BORN_FROM_BELIEFS
// % modifier to final unit production cost total (useful for settlers which have variable costs depending on start era)
#define NQ_UNIT_FINAL_PRODUCTION_COST_MODIFIER
// defense from Citizens as a stat for buildings
#define NQ_BUILDING_DEFENSE_FROM_CITIZENS
// free great work in this building if built in capital
#define NQ_CHEAT_FIRST_ROYAL_LIBRARY_COMES_WITH_GREAT_WORK
// trait that makes long term city state allies refuse bribes
#define	NQ_NUM_TURNS_BEFORE_MINOR_ALLIES_REFUSE_BRIBES_FROM_TRAIT
// trait attribute that make specific unit types immune to plunder mission
#define NQ_UNIT_IMMUNE_TO_PLUNDER_FROM_TRAIT
// fixing build times (sub-part of Delnar's fixes)
#define NQ_FIX_BUILD_TIMES_UI
// make build times round down to lowest 10 so that Quick Speed doesn't have built times of 201 or 402
#define NQ_ROUND_BUILD_TIMES_DOWN
// HACK: Golden Pilgrimage Trait (new civ Mali) needs to give +1 Faith to tiles that make at least 1 Gold during a golden age
#define NQ_GOLDEN_PILGRIMAGE
// HACK: Mali Treasury needs to be in C++ instead of Lua to fix the resync issue
#define NQ_MALI_TREASURY
// allow gifting faith to minors via beliefs
#define NQ_BELIEF_TOGGLE_ALLOW_FAITH_GIFTS_TO_MINORS


/// ---- IDEOLOGY OVERHAUL: AUTOCRACY ----
// influence gain on gold tribute from policies
#define NQ_MINOR_FRIENDSHIP_GAIN_BULLY_GOLD_SUCCESS_FROM_POLICIES
// flat coup chance modifier
#define NQ_COUP_CHANCE_MODIFIER_FROM_POLICIES
// attack bonus in foreign lands during golden ages
#define NQ_GOLDEN_AGE_FOREIGN_ATTACK_BONUS
// with this policy, gain a free great artist whenever a great general is born (very ugly cheat code)
#define NQ_WAR_HERO
// ignore puppets when calculating tech research cost
#define NQ_IGNORE_PUPPETS_FOR_RESEARCH_COSTS_FROM_POLICIES
// allows purchasing of units in puppet cities from policies
#define NQ_ALLOW_PUPPET_PURCHASING_FROM_POLICIES
// bonus strength when near friendly city states or city states conquered by another civ
#define NQ_COMBAT_STRENGTH_NEAR_FRIENDLY_MINOR


/// ---- IDEOLOGY OVERHAUL: FREEDOM ----
// trade mission influence modifier from policies
#define NQ_TRADE_MISSION_INFLUENCE_MODIFIER_FROM_POLICIES
// science per great person born from policies
#define NQ_SCIENCE_PER_GREAT_PERSON_BORN_FROM_POLICIES
// influence with all known city states per great person born from policies
#define NQ_INFLUENCE_BOOST_PER_GREAT_PERSON_BORN_FROM_POLICIES
// gold to science conversion ratio at empire level from policies
#define NQ_GOLD_TO_SCIENCE_FROM_POLICIES
// tourism with all known civs on trade missions from policies
#define NQ_TOURISM_FROM_TRADE_MISSIONS_FROM_POLICIES
// happiness from great person tile improvements from policies
#define NQ_HAPPINESS_FROM_GREAT_IMPROVEMENTS_FROM_POLICIES
// prevent friendly minors declaring war on you in support of their ally, policy toggle
#define NQ_POLICY_TOGGLE_NO_MINOR_DOW_IF_FRIENDS

/// ---- IDEOLOGY OVERHAUL: ORDER ----
// tourism per city from policies
#define NQ_TOURISM_PER_CITY
// extra spies from policies
#define NQ_EXTRA_SPIES_FROM_POLICIES
// production from internal trade routes from policies
#define NQ_INTERNAL_TRADE_ROUTE_PRODUCTION_YIELD_CHANGE_FROM_POLICIES
// disables resistance time via policies
#define NQ_DIABLE_RESISTANCE_TIME_VIA_POLICIES
// unhappiness modifier for ideology pressure from policies
#define NQ_IDEOLOGY_PRESSURE_UNHAPPINESS_MODIFIER_FROM_POLICIES
// happineses from railway connections
#define NQ_RAIL_CONNECTION_HAPPINESS_FROM_POLICIES
// bonus production modifier from railway connections
#define NQ_RAIL_CONNECTION_PRODUCTION_MODIFIER_FROM_POLICIES
// policy toggle, when true makes each military land unit trained result in 2 units instead of 1
#define NQ_PATRIOTIC_WAR
// combat bonus vs smaller civ from policies
#define NQ_COMBAT_BONUS_VS_SMALLER_CIV_FROM_POLICIES
// percent of production converted into bonus % for great musician points
#define NQ_PRODUCTION_TO_GREAT_MUSICIANS_MODIFIER_FROM_POLICIES


/// -- MORE OPTIONS FOR v12.3 AND ON ---
// remove the diplomatic penalty for expanding like crazy
#define NQ_GAME_OPTION_DISABLE_RECKLESS_EXPANDER*/
// notifications of city growth occur at all pop levels (not just less than 5)
/////#define NQ_ALWAYS_SHOW_POP_GROWTH_NOTIFICATION
/*// tributing now relies more on local military power rather than global
#define NQ_TRIBUTE_EASIER_WITH_LOCAL_POWER*/
// We Love The King Day now scales based on game speed
/////#define NQ_WLTKD_SCALES_BY_GAME_SPEED
/*// We Love The King Day initial seed now scales with game speed
#define NQ_WLTKD_SEED_SCALES_WITH_GAME_SPEED*/
// Trade Route duration now scales with game speed
/////#define NQ_TRADE_ROUTE_DURATION_SCALES_WITH_GAME_SPEED
/*// Remove random extra turns from seed of WTLKD resource demand
#define NQ_DISABLE_WLTKD_SEED_RANDOM_EXTRA_TURNS
// Adjacent Mod (PROMOTION_ADJACENT_BONUS) now requires same combat type
#define NQ_ADJACENT_MOD_REQUIRES_SAME_COMBAT_TYPE
*/

#endif