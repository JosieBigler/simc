// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Heal
// ==========================================================================

// ==========================================================================
// Created by philoptik@gmail.com
//
// heal_target is set to player for now.
// ==========================================================================

// heal_t::heal_t ======== Heal Constructor ===================

heal_t::heal_t( const std::string&  token,
                player_t*           p,
                const spell_data_t* s ) :
  spell_base_t( ACTION_HEAL, token, p, s )
{
  if ( target -> is_enemy() || target -> is_add() )
    target = player;

  group_only = false;

  dot_behavior      = DOT_REFRESH;
  weapon_multiplier = 0.0;
  may_crit          = true;
  tick_may_crit     = true;

  stats -> type = STATS_HEAL;

  crit_bonus = 1.0;

  crit_multiplier = 1.0;

  if ( player -> meta_gem == META_REVITALIZING_SHADOWSPIRIT )
  {
    crit_multiplier *= 1.03;
  }
}

// heal_t::execute ==========================================================

void heal_t::execute()
{
  spell_base_t::execute();

  if ( callbacks )
  {
    if ( ! is_tick_action && result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> callbacks.heal[ result ], this );
    }
    if ( ! background ) // OnSpellCast
    {
      action_callback_t::trigger( player -> callbacks.heal[ RESULT_NONE ], this );
    }
  }
}

// heal_t::assess_damage ====================================================

void heal_t::assess_damage( dmg_e heal_type,
                            action_state_t* s )
{
  player_t::heal_info_t heal = s -> target -> assess_heal( s -> result_amount, school, heal_type, s -> result, this );

  if ( heal_type == HEAL_DIRECT )
  {
    if ( sim -> log )
    {
      sim -> output( "%s %s heals %s for %.0f (%.0f) (%s)",
                     player -> name(), name(),
                     s -> target -> name(), heal.amount, heal.actual,
                     util::result_type_string( s -> result ) );
    }

    if ( callbacks && ! direct_tick_callbacks ) action_callback_t::trigger( player -> callbacks.direct_heal[ school ], this, s );
    if ( direct_tick_callbacks ) action_callback_t::trigger( player -> callbacks.tick_heal[ school ], this, s );
  }
  else // HEAL_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = get_dot( s -> target );
      sim -> output( "%s %s ticks (%d of %d) %s for %.0f (%.0f) heal (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     s -> target -> name(), heal.amount, heal.actual,
                     util::result_type_string( s -> result ) );
    }

    if ( callbacks ) action_callback_t::trigger( player -> callbacks.tick_heal[ school ], this, s );
  }

  stats -> add_result( sim -> report_overheal ? heal.amount : heal.actual, heal.actual, ( direct_tick ? HEAL_OVER_TIME : heal_type ), s -> result );
}

// heal_t::find_greatest_difference_player ==================================

player_t* heal_t::find_greatest_difference_player()
{
  double diff=0;
  double max=0;
  player_t* max_player = player;
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    // No love for pets right now
    diff = p -> is_pet() ? 0 : p -> resources.max[ RESOURCE_HEALTH ] - p -> resources.current[ RESOURCE_HEALTH ];
    if ( diff > max )
    {
      max = diff;
      max_player = p;
    }
  }
  return max_player;
}

// heal_t::find_lowest_player ===============================================

player_t* heal_t::find_lowest_player()
{
  double diff=0;
  double max=0;
  player_t* max_player = player;
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    // No love for pets right now
    diff =  p -> is_pet() ? 0 : 1.0 / p -> resources.current[ RESOURCE_HEALTH ];
    if ( diff > max )
    {
      max = diff;
      max_player = p;
    }
  }
  return max_player;
}

// heal_t::available_targets ==============================================

size_t heal_t::available_targets( std::vector< player_t* >& tl )
{
  // TODO: This does not work for heals at all, as it presumes enemies in the
  // actor list.

  if ( group_only )
  {
    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
           ! sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] != target && sim -> actor_list[ i ] -> party == target -> party )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

  return spell_base_t::available_targets( tl );
}
