/*
 *  File:       describe.cc
 *  Summary:    Functions used to print information about various game objects.
 *  Written by: Linley Henzell
 *
 *  Modified for Crawl Reference by $Author$ on $Date$
 *
 *  Change History (most recent first):
 *
 *      <4>      10/14/99     BCR     enummed describe_god()
 *      <3>      10/13/99     BCR     Added GOD_NO_GOD case in describe_god()
 *      <2>      5/20/99      BWR     Replaced is_artifact with
 *                                    is_dumpable_artefact
 *      <1>      4/20/99      JDJ     Reformatted, uses string objects,
 *                                    split out 10 new functions from
 *                                    describe_item(), added
 *                                    get_item_description and
 *                                    is_artifact.
 */

#include "AppHdr.h"
#include "describe.h"
#include "database.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <iomanip>

#ifdef DOS
#include <conio.h>
#endif

#include "externs.h"

#include "abl-show.h"
#include "cio.h"
#include "debug.h"
#include "decks.h"
#include "fight.h"
#include "food.h"
#include "it_use2.h"
#include "itemname.h"
#include "itemprop.h"
#include "macro.h"
#include "menu.h"
#include "message.h"
#include "mon-util.h"
#include "player.h"
#include "randart.h"
#include "religion.h"
#include "skills2.h"
#include "spl-book.h"
#include "stuff.h"
#include "spl-util.h"
#include "tutorial.h"


// ========================================================================
//      Internal Functions
// ========================================================================

//---------------------------------------------------------------
//
// append_value
//
// Appends a value to the string. If plussed == 1, will add a + to
// positive values (itoa always adds - to -ve ones).
//
//---------------------------------------------------------------
static void append_value( std::string & description, int valu, bool plussed )
{
    if (valu >= 0 && plussed == 1)
        description += "+";

    char value_str[80];

    itoa( valu, value_str, 10 );

    description += value_str;
}                               // end append_value()

//---------------------------------------------------------------
//
// print_description
//
// Takes a descrip string filled up with stuff from other functions,
// and displays it with minor formatting to avoid cut-offs in mid
// word and such. The character $ is interpreted as a CR.
//
//---------------------------------------------------------------
void print_description( const std::string &d )
{
    std::string::size_type nextLine = std::string::npos;
    unsigned int  currentPos = 0;

    const unsigned int lineWidth = get_number_of_cols() - 1;

    textcolor(LIGHTGREY);

    while(currentPos < d.length())
    {
        if (currentPos != 0)
            gotoxy(1, wherey() + 1);

        // see if $ sign is within one lineWidth
        nextLine = d.find('$', currentPos);

        if (nextLine >= currentPos && nextLine < currentPos + lineWidth)
        {
            cprintf("%s", (d.substr(currentPos, nextLine-currentPos)).c_str());
            currentPos = nextLine + 1;
            continue;
        }
        
        // Handle real line breaks.  No substitutions necessary, just update
        // the counts.
        nextLine = d.find('\n', currentPos);
        if (nextLine >= currentPos && nextLine < currentPos + lineWidth)
        {
            cprintf("%s", (d.substr(currentPos, nextLine-currentPos)).c_str());
            currentPos = nextLine +1;
            continue;
        }            
        
        // no newline -- see if rest of string will fit.
        if (currentPos + lineWidth >= d.length())
        {
            cprintf((d.substr(currentPos)).c_str());
            return;
        }


        // ok.. try to truncate at space.
        nextLine = d.rfind(' ', currentPos + lineWidth);

        if (nextLine > 0)
        {
            cprintf((d.substr(currentPos, nextLine - currentPos)).c_str());
            currentPos = nextLine + 1;
            continue;
        }

        // oops.  just truncate.
        nextLine = currentPos + lineWidth;

        if (nextLine > d.length())
            nextLine = d.length();

        cprintf((d.substr(currentPos, nextLine - currentPos)).c_str());
        currentPos = nextLine;
    }
}

//---------------------------------------------------------------
//
// randart_descrip
//
// Appends the various powers of a random artefact to the description
// string.
//
//---------------------------------------------------------------
static void randart_descrip( std::string &description, const item_def &item )
{
    unsigned int old_length = description.length();

    randart_properties_t proprt;
    randart_wpn_properties( item, proprt );

    if (proprt[ RAP_AC ])
    {
        description += "$It affects your AC (";
        append_value(description, proprt[ RAP_AC ], true);
        description += ").";
    }

    if (proprt[ RAP_EVASION ])
    {
        description += "$It affects your evasion (";
        append_value(description, proprt[ RAP_EVASION ], true);
        description += ").";
    }

    if (proprt[ RAP_STRENGTH ])
    {
        description += "$It affects your strength (";
        append_value(description, proprt[ RAP_STRENGTH ], true);
        description += ").";
    }

    if (proprt[ RAP_INTELLIGENCE ])
    {
        description += "$It affects your intelligence (";
        append_value(description, proprt[ RAP_INTELLIGENCE ], true);
        description += ").";
    }

    if (proprt[ RAP_DEXTERITY ])
    {
        description += "$It affects your dexterity (";
        append_value(description, proprt[ RAP_DEXTERITY ], true);
        description += ").";
    }

    if (proprt[ RAP_ACCURACY ])
    {
        description += "$It affects your accuracy (";
        append_value(description, proprt[ RAP_ACCURACY ], true);
        description += ").";
    }

    if (proprt[ RAP_DAMAGE ])
    {
        description += "$It affects your damage-dealing abilities (";
        append_value(description, proprt[ RAP_DAMAGE ], true);
        description += ").";
    }

    if (proprt[ RAP_FIRE ] < -2)
        description += "$It makes you extremely vulnerable to fire. ";
    else if (proprt[ RAP_FIRE ] == -2)
        description += "$It makes you very vulnerable to fire. ";
    else if (proprt[ RAP_FIRE ] == -1)
        description += "$It makes you vulnerable to fire. ";
    else if (proprt[ RAP_FIRE ] == 1)
        description += "$It protects you from fire. ";
    else if (proprt[ RAP_FIRE ] == 2)
        description += "$It greatly protects you from fire. ";
    else if (proprt[ RAP_FIRE ] > 2)
        description += "$It renders you almost immune to fire. ";

    if (proprt[ RAP_COLD ] < -2)
        description += "$It makes you extremely vulnerable to cold. ";
    else if (proprt[ RAP_COLD ] == -2)
        description += "$It makes you very vulnerable to cold. ";
    else if (proprt[ RAP_COLD ] == -1)
        description += "$It makes you vulnerable to cold. ";
    else if (proprt[ RAP_COLD ] == 1)
        description += "$It protects you from cold. ";
    else if (proprt[ RAP_COLD ] == 2)
        description += "$It greatly protects you from cold. ";
    else if (proprt[ RAP_COLD ] > 2)
        description += "$It renders you almost immune to cold. ";

    if (proprt[ RAP_ELECTRICITY ])
        description += "$It insulates you from electricity. ";

    if (proprt[ RAP_POISON ])
        description += "$It protects you from poison. ";

    if (proprt[ RAP_NEGATIVE_ENERGY ] == 1)
        description += "$It partially protects you from negative energy. ";
    else if (proprt[ RAP_NEGATIVE_ENERGY ] == 2)
        description += "$It protects you from negative energy. ";
    else if (proprt[ RAP_NEGATIVE_ENERGY ] > 2)
        description += "$It renders you almost immune to negative energy. ";

    if (proprt[ RAP_MAGIC ])
        description += "$It increases your resistance to enchantments. ";

    if (proprt[ RAP_STEALTH ] < 0)
    {
        if (proprt[ RAP_STEALTH ] < -20)
            description += "$It makes you much less stealthy. ";
        else
            description += "$It makes you less stealthy. ";
    }
    else if (proprt[ RAP_STEALTH ] > 0)
    {
        if (proprt[ RAP_STEALTH ] > 20)
            description += "$It makes you much more stealthy. ";
        else
            description += "$It makes you more stealthy. ";
    }

    if (proprt[ RAP_EYESIGHT ])
        description += "$It enhances your eyesight. ";

    if (proprt[ RAP_INVISIBLE ])
        description += "$It lets you turn invisible. ";

    if (proprt[ RAP_LEVITATE ])
        description += "$It lets you levitate. ";

    if (proprt[ RAP_BLINK ])
        description += "$It lets you blink. ";

    if (proprt[ RAP_CAN_TELEPORT ])
        description += "$It lets you teleport. ";

    if (proprt[ RAP_BERSERK ])
        description += "$It lets you go berserk. ";

    if (proprt[ RAP_MAPPING ])
        description += "$It lets you sense your surroundings. ";

    if (proprt[ RAP_NOISES ])
        description += "$It makes noises. ";

    if (proprt[ RAP_PREVENT_SPELLCASTING ])
        description += "$It prevents spellcasting. ";

    if (proprt[ RAP_CAUSE_TELEPORTATION ])
        description += "$It causes teleportation. ";

    if (proprt[ RAP_PREVENT_TELEPORTATION ])
        description += "$It prevents most forms of teleportation. ";

    if (proprt[ RAP_ANGRY ])
        description += "$It makes you angry. ";

    if (proprt[ RAP_METABOLISM ] >= 3)
        description += "$It greatly speeds your metabolism. ";
    else if (proprt[ RAP_METABOLISM ])
        description += "$It speeds your metabolism. ";

    if (proprt[ RAP_MUTAGENIC ] > 3)
        description += "$It glows with mutagenic radiation.";
    else if (proprt[ RAP_MUTAGENIC ])
        description += "$It emits mutagenic radiation.";

    if (old_length != description.length())
        description += "$";

    if (is_unrandom_artefact( item ))
    {
        const char *desc = unrandart_descrip( 0, item );
        if (strlen( desc ) > 0)
        {
            description += desc;
            description += "$";
        }
    }
}

static const char *trap_names[] =
{
    "dart", "arrow", "spear", "axe",
    "teleport", "amnesia", "blade",
    "bolt", "net", "zot", "needle",
};

const char *trap_name(trap_type trap)
{
    ASSERT(NUM_TRAPS == sizeof(trap_names) / sizeof(*trap_names));
    
    if (trap >= TRAP_DART && trap < NUM_TRAPS)
        return trap_names[ static_cast<int>( trap ) ];
    return (NULL);
}

int str_to_trap(const std::string &s)
{
    ASSERT(NUM_TRAPS == sizeof(trap_names) / sizeof(*trap_names));
    
    if (s == "random" || s == "any")
        return (TRAP_RANDOM);
    else if (s == "suitable")
        return (TRAP_INDEPTH);

    for (int i = 0; i < NUM_TRAPS; ++i)
    {
        if (trap_names[i] == s)
            return (i);
    }

    return (-1);
}

//---------------------------------------------------------------
//
// describe_demon
//
// Describes the random demons you find in Pandemonium.
//
//---------------------------------------------------------------
static std::string describe_demon(const monsters &mons)
{
    ASSERT(mons.ghost.get());
    
    long globby = 0;
    const ghost_demon &ghost = *mons.ghost;
    
    for (unsigned i = 0; i < ghost.name.length(); i++)
        globby += ghost.name[i];

    globby *= ghost.name.length();

    rng_save_excursion exc;
    seed_rng( globby );

    std::string description = "A powerful demon, ";

    description += ghost.name;
    description += " has a";

    switch (random2(31))
    {
    case 0:
        description += " huge, barrel-shaped ";
        break;
    case 1:
        description += " wispy, insubstantial ";
        break;
    case 2:
        description += " spindly ";
        break;
    case 3:
        description += " skeletal ";
        break;
    case 4:
        description += " horribly deformed ";
        break;
    case 5:
        description += " spiny ";
        break;
    case 6:
        description += " waif-like ";
        break;
    case 7:
        description += " scaly ";
        break;
    case 8:
        description += " sickeningly deformed ";
        break;
    case 9:
        description += " bruised and bleeding ";
        break;
    case 10:
        description += " sickly ";
        break;
    case 11:
        description += " mass of writhing tentacles for a ";
        break;
    case 12:
        description += " mass of ropey tendrils for a ";
        break;
    case 13:
        description += " tree trunk-like ";
        break;
    case 14:
        description += " hairy ";
        break;
    case 15:
        description += " furry ";
        break;
    case 16:
        description += " fuzzy ";
        break;
    case 17:
        description += "n obese ";
        break;
    case 18:
        description += " fat ";
        break;
    case 19:
        description += " slimy ";
        break;
    case 20:
        description += " wrinkled ";
        break;
    case 21:
        description += " metallic ";
        break;
    case 22:
        description += " glassy ";
        break;
    case 23:
        description += " crystalline ";
        break;
    case 24:
        description += " muscular ";
        break;
    case 25:
        description += "n icky ";
        break;
    case 26:
        description += " swollen ";
        break;
    case 27:
        description += " lumpy ";
        break;
    case 28:
        description += "n armoured ";
        break;
    case 29:
        description += " carapaced ";
        break;
    case 30:
        description += " slender ";
        break;
    }

    description += "body";

    
    switch (ghost.values[GVAL_DEMONLORD_FLY])
    {
    case 1: // proper flight
        switch (random2(10))
        {
        case 0:
            description += " with small insectoid wings";
            break;
        case 1:
            description += " with large insectoid wings";
            break;
        case 2:
            description += " with moth-like wings";
            break;
        case 3:
            description += " with butterfly wings";
            break;
        case 4:
            description += " with huge, bat-like wings";
            break;
        case 5:
            description += " with fleshy wings";
            break;
        case 6:
            description += " with small, bat-like wings";
            break;
        case 7:
            description += " with hairy wings";
            break;
        case 8:
            description += " with great feathered wings";
            break;
        case 9:
            description += " with shiny metal wings";
            break;
        default:
            break;
        }
        break;

    case 2: // levitation
        if (coinflip())
            description += " which hovers in mid-air";
        else
            description += " with sacs of gas hanging from its back";
        break;

    default:  // does not fly
        switch (random2(40))
        {
        default:
            break;
        case 12:
            description += " covered in tiny crawling spiders";
            break;
        case 13:
            description += " covered in tiny crawling insects";
            break;
        case 14:
            description += " and the head of a crocodile";
            break;
        case 15:
            description += " and the head of a hippopotamus";
            break;
        case 16:
            description += " and a cruel curved beak for a mouth";
            break;
        case 17:
            description += " and a straight sharp beak for a mouth";
            break;
        case 18:
            description += " and no head at all";
            break;
        case 19:
            description += " and a hideous tangle of tentacles for a mouth";
            break;
        case 20:
            description += " and an elephantine trunk";
            break;
        case 21:
            description += " and an evil-looking proboscis";
            break;
        case 22:
            description += " and dozens of eyes";
            break;
        case 23:
            description += " and two ugly heads";
            break;
        case 24:
            description += " and a long serpentine tail";
            break;
        case 25:
            description += " and a pair of huge tusks growing from its jaw";
            break;
        case 26:
            description +=
                " and a single huge eye, in the centre of its forehead";
            break;
        case 27:
            description += " and spikes of black metal for teeth";
            break;
        case 28:
            description += " and a disc-shaped sucker for a head";
            break;
        case 29:
            description += " and huge, flapping ears";
            break;
        case 30:
            description += " and a huge, toothy maw in the centre of its chest";
            break;
        case 31:
            description += " and a giant snail shell on its back";
            break;
        case 32:
            description += " and a dozen heads";
            break;
        case 33:
            description += " and the head of a jackal";
            break;
        case 34:
            description += " and the head of a baboon";
            break;
        case 35:
            description += " and a huge, slobbery tongue";
            break;
        case 36:
            description += " which is covered in oozing lacerations";
            break;
        case 37:
            description += " and the head of a frog";
            break;
        case 38:
            description += " and the head of a yak";
            break;
        case 39:
            description += " and eyes out on stalks";
            break;
        }
        break;
    }

    description += ".";

    switch (random2(40) + (player_can_smell() ? 0 : 3))
    {
    case 0:
        description += " It stinks of brimstone.";
        break;
    case 1:
        description += " It smells like rotting flesh";
        if (you.species == SP_GHOUL)
            description += " - yum!";
        else
            description += ".";
        break;
    case 2:
        description += " It is surrounded by a sickening stench.";
        break;
    case 3:
        description += " It seethes with hatred of the living.";
        break;
    case 4:
        description += " Tiny orange flames dance around it.";
        break;
    case 5:
        description += " Tiny purple flames dance around it.";
        break;
    case 6:
        description += " It is surrounded by a weird haze.";
        break;
    case 7:
        description += " It glows with a malevolent light.";
        break;
    case 8:
        description += " It looks incredibly angry.";
        break;
    case 9:
        description += " It oozes with slime.";
        break;
    case 10:
        description += " It dribbles constantly.";
        break;
    case 11:
        description += " Mould grows all over it.";
        break;
    case 12:
        description += " It looks diseased.";
        break;
    case 13:
        description += " It looks as frightened of you as you are of it.";
        break;
    case 14:
        description += " It moves in a series of hideous convulsions.";
        break;
    case 15:
        description += " It moves with an unearthly grace.";
        break;
    case 16:
        description += " It hungers for your soul!";
        break;
    case 17:
        description += " It leaves a glistening oily trail.";
        break;
    case 18:
        description += " It shimmers before your eyes.";
        break;
    case 19:
        description += " It is surrounded by a brilliant glow.";
        break;
    case 20:
        description += " It radiates an aura of extreme power.";
        break;
    default:
        break;
    }
    return description;
}                               // end describe_demon()


//---------------------------------------------------------------
//
// describe_weapon
//
//---------------------------------------------------------------
static std::string describe_weapon( const item_def &item, bool verbose)
{
    std::string description;

    description.reserve(200);

    description = "";

    if (is_fixed_artefact( item ))
    {
        if (item_ident( item, ISFLAG_KNOW_PROPERTIES ))
        {
            description += "$";

            switch (item.special)
            {
            case SPWPN_SINGING_SWORD:
                description += "This blessed weapon loves nothing more "
                    "than to sing to its owner, "
                    "whether they want it to or not. ";
                break;
            case SPWPN_WRATH_OF_TROG:
                description += "This was the favourite weapon of "
                    "the old god Trog, before it was lost one day. "
                    "It induces a bloodthirsty berserker rage in "
                    "anyone who uses it to strike another. ";
                break;
            case SPWPN_SCYTHE_OF_CURSES:
                description += "This weapon carries a "
                    "terrible and highly irritating curse. ";
                break;
            case SPWPN_MACE_OF_VARIABILITY:
                description += "It is rather unreliable. ";
                break;
            case SPWPN_GLAIVE_OF_PRUNE:
                description += "It is the creation of a mad god, and "
                    "carries a curse which transforms anyone "
                    "possessing it into a prune. Fortunately, "
                    "the curse works very slowly, and one can "
                    "use it briefly with no consequences "
                    "worse than slightly purple skin and a few wrinkles. ";
                break;
            case SPWPN_SCEPTRE_OF_TORMENT:
                description += "This truly accursed weapon is "
                    "an instrument of Hell. ";
                break;
            case SPWPN_SWORD_OF_ZONGULDROK:
                description += "This dreadful weapon is used "
                    "at the user's peril. ";
                break;
            case SPWPN_SWORD_OF_CEREBOV:
                description += "Eerie flames cover its twisted blade. ";
                break;
            case SPWPN_STAFF_OF_DISPATER:
                description += "This legendary item can unleash "
                    "the fury of Hell. ";
                break;
            case SPWPN_SCEPTRE_OF_ASMODEUS:
                description += "It carries some of the powers of "
                    "the arch-fiend Asmodeus. ";
                break;
            case SPWPN_SWORD_OF_POWER:
                description += "It rewards the powerful with power "
                    "and the meek with weakness. ";
                break;
            case SPWPN_KNIFE_OF_ACCURACY:
                description += "It is almost unerringly accurate. ";
                break;
            case SPWPN_STAFF_OF_OLGREB:
                description += "It was the magical weapon wielded by the "
                    "mighty wizard Olgreb before he met his "
                    "fate somewhere within these dungeons. It "
                    "grants its wielder resistance to the "
                    "effects of poison and increases their "
                    "ability to use venomous magic, and "
                    "carries magical powers which can be evoked. ";
                break;
            case SPWPN_VAMPIRES_TOOTH:
                description += "It is lethally vampiric. ";
                break;
            case SPWPN_STAFF_OF_WUCAD_MU:
                description += "Its power varies in proportion to "
                    "its wielder's intelligence. "
                    "Using it can be a bit risky. ";
                break;
            }

            description += "$";
        }
        else if (item_type_known(item))
        {
            // We know it's an artefact type weapon, but not what it does.
            description += "$This weapon may have some hidden properties.$";
        }
    }
    else if (is_unrandom_artefact( item )
        && strlen(unrandart_descrip(1, item)) != 0)
    {
        description += unrandart_descrip(1, item);
        description += "$";
    }
    else
    {
        if (verbose)
        {
            switch (item.sub_type)
            {
            case WPN_CLUB:
                description += "A heavy piece of wood. ";
                break;

            case WPN_MACE:
                description += "A long handle "
                    "with a heavy lump on one end. ";
                break;

            case WPN_FLAIL:
                description += "Like a mace, but with a length of chain "
                    "between the handle and the lump of metal. ";
                break;

            case WPN_DAGGER:
                description += "A long knife or a very short sword, "
                    "which can be held or thrown. ";
                break;

            case WPN_KNIFE:
                description += "A simple survival knife. "
                    "Designed more for utility than combat, "
                    "it looks quite capable of butchering a corpse. ";
                break;

            case WPN_MORNINGSTAR:
                description += "A mace covered in spikes. ";
                break;

            case WPN_SHORT_SWORD:
                description += "A sword with a short, slashing blade. ";
                break;

            case WPN_LONG_SWORD:
                description += "A sword with a long, slashing blade. ";
                break;

            case WPN_GREAT_SWORD:
                description += "A sword with a very long, heavy blade "
                    "and a long handle. ";
                break;

            case WPN_SCIMITAR:
                description += "A long sword with a curved blade. ";
                break;

            case WPN_HAND_AXE:
                description += "A small axe designed for either hand combat "
                               "or throwing. ";
                break;

            case WPN_BATTLEAXE:
                description += "A large axe with a double-headed blade. ";
                break;

            case WPN_SPEAR:
                description += "A long stick with a pointy blade on one end, "
                    "to be held or thrown. ";
                break;

            case WPN_TRIDENT:
                description +=
                    "A hafted weapon with three points at one end. ";
                break;

            case WPN_HALBERD:
                description +=
                    "A long pole with a spiked axe head on one end. ";
                break;

            case WPN_SLING:
                description +=
                    "A piece of cloth and leather for launching stones, "
                    "which do a small amount of damage on impact. ";
                break;

            case WPN_BOW:
                description += "A curved piece of wood and string, "
                    "for shooting arrows. It does good damage in combat, "
                    "and a skilled user can use it to great effect. ";
                break;

            case WPN_LONGBOW:
                description += "A long, strong bow made of yew. "
                    "It does excellent damage in combat "
                    "and a skilled archer can use it to great effect. ";
                break;

            case WPN_BLOWGUN:
                description += "A long, light tube, open at both ends.  Doing "
                    "very little damage, its main use is to fire poisoned "
                    "needles from afar.  It makes very little noise. ";
                break;

            case WPN_CROSSBOW:
                description += "A piece of machinery used for firing bolts, "
                    "which takes some time to load and fire. "
                    "It does very good damage in combat. ";
                break;

            case WPN_HAND_CROSSBOW:
                description += "A small crossbow, for firing darts. ";
                break;

            case WPN_GLAIVE:
                description +=
                    "A pole with a large, heavy blade on one end. ";
                break;

            case WPN_QUARTERSTAFF:
                description += "A sturdy wooden pole. ";
                break;

            case WPN_SCYTHE:
                description +=
                    "A farm implement, usually unsuited to combat. ";
                break;

            case WPN_GIANT_CLUB:
                description += "A giant lump of wood, "
                    "shaped for an ogre's hands. ";
                break;

            case WPN_GIANT_SPIKED_CLUB:
                description +=
                    "A giant lump of wood with sharp spikes at one end. ";
                break;

            case WPN_EVENINGSTAR:
                description += "The opposite of a morningstar. ";
                break;

            case WPN_QUICK_BLADE:
                description += "A small and magically quick sword. ";
                break;

            case WPN_KATANA:
                description += "A very rare and extremely effective "
                    "imported weapon, featuring a long "
                    "single-edged blade. ";
                break;

            case WPN_LAJATANG:
                description += "A very rare and extremely effective "
                    "imported weapon, featuring a pole with half-moon blades "
                    "at both ends. ";
                break;

            case WPN_LOCHABER_AXE:
                description += "An enormous combination of a pike "
                               "and a battle axe.";
                break;

            case WPN_EXECUTIONERS_AXE:
                description += "A huge axe. ";
                break;

            case WPN_DOUBLE_SWORD:
                description +=
                    "A magical weapon with two razor-sharp blades. ";
                break;

            case WPN_TRIPLE_SWORD:
                description += "A magical weapon with three "
                    "great razor-sharp blades. ";
                break;

            case WPN_HAMMER:
                description += "The kind of thing you hit nails with, "
                    "adapted for battle. ";
                break;

            case WPN_ANCUS:
                description += "A large and vicious toothed club. ";
                break;

            case WPN_WHIP:
                description += "A whip. ";
                break;

            case WPN_SABRE:
                description += "A sword with a medium length slashing blade. ";
                break;

            case WPN_DEMON_BLADE:
                description +=
                    "A terrible weapon, forged in the fires of Hell. ";
                break;

            case WPN_BLESSED_BLADE:
                description += "A blade blessed by the Shining One. ";
                break;

            case WPN_DEMON_WHIP:
                description += "A terrible weapon, woven "
                    "in the depths of the inferno. ";
                break;

            case WPN_DEMON_TRIDENT:
                description +=
                    "A terrible weapon, molded by fire and brimstone. ";
                break;

            case WPN_BROAD_AXE:
                description += "An axe with a large blade. ";
                break;

            case WPN_WAR_AXE:
                description += "An axe intended for hand to hand combat. ";
                break;

            case WPN_SPIKED_FLAIL:
                description +=
                    "A flail with large spikes on the metal lump. ";
                break;

            case WPN_GREAT_MACE:
                description += "A large and heavy mace. ";
                break;

            case WPN_DIRE_FLAIL:
                description += "A flail with spiked lumps on both ends.";
                break;

            case WPN_FALCHION:
                description += "A sword with a broad slashing blade. ";
                break;

            default:
                DEBUGSTR("Unknown weapon");
            }

            description += "$";
        }
    }

    if (verbose)
    {
        description += "$Damage rating: ";
        append_value(description, property( item, PWPN_DAMAGE ), false);

        description += "$Accuracy rating: ";
        append_value(description, property( item, PWPN_HIT ), true);

        description += "$Base attack delay: ";
        append_value(description, property( item, PWPN_SPEED ) * 10, false);
        description += "%";
    }
    description += "$";

    if (!is_fixed_artefact( item ))
    {
        int spec_ench = get_weapon_brand( item );

        if (!is_random_artefact( item ) && !verbose)
            spec_ench = SPWPN_NORMAL;

        // special weapon descrip
        if (spec_ench != SPWPN_NORMAL && item_type_known(item))
        {
            description += "$";

            switch (spec_ench)
            {
            case SPWPN_FLAMING:
                description += "It emits flame when wielded, "
                    "causing extra injury to most foes "
                    "and up to double damage against "
                    "particularly susceptible opponents. ";
                break;
            case SPWPN_FREEZING:
                description += "It has been specially enchanted to "
                    "freeze those struck by it, causing "
                    "extra injury to most foes and "
                    "up to double damage against "
                    "particularly susceptible opponents. ";
                break;
            case SPWPN_HOLY_WRATH:
                description += "It has been blessed by the Shining One "
                    "to harm undead and cause great damage to "
                    "the unholy creatures of Hell or Pandemonium. ";
                break;
            case SPWPN_ELECTROCUTION:
                description += "Occasionally upon striking a foe "
                    "it will discharge some electrical energy "
                    "and cause terrible harm. ";
                break;
            case SPWPN_ORC_SLAYING:
                description += "It is especially effective against "
                    "all of orcish descent. ";
                break;
            case SPWPN_VENOM:
                if (is_range_weapon(item))
                    description += "It poisons the unbranded ammo it fires. ";
                else
                    description += "It poisons the flesh of those it strikes. ";
                break;
            case SPWPN_PROTECTION:
                description += "It protects the one who wields it against "
                    "injury (+5 to AC). ";
                break;
            case SPWPN_DRAINING:
                description += "A truly terrible weapon, "
                    "it drains the life of those it strikes. ";
                break;
            case SPWPN_SPEED:
                if (is_range_weapon(item))
                {
                    description += "It allows its wielder to fire twice when "
                           "they would otherwise have fired only once. ";
                }
                else
                {
                    description += "It allows its wielder to attack twice when "
                           "they would otherwise have struck only once. ";
                }
                break;
            case SPWPN_VORPAL:
                if (is_range_weapon(item))
                {
                    description += "Any ";
                    description += ammo_name( item );
                    description += " fired from it inflicts extra damage."; 
                }
                else
                {
                    description += "It inflicts extra damage upon your enemies. ";
                }
                break;
            case SPWPN_FLAME:
                description += "It turns projectiles fired from it into "
                    "bolts of fire. ";
                break;
            case SPWPN_FROST:
                description += "It turns projectiles fired from it into "
                    "bolts of frost. ";
                break;
            case SPWPN_VAMPIRICISM:
                description += "It inflicts no extra harm, "
                    "but heals its wielder somewhat when "
                    "it strikes a living foe. ";
                break;
            case SPWPN_DISRUPTION:
                description += "It is a weapon blessed by Zin, "
                    "and can inflict up to fourfold damage "
                    "when used against the undead. ";
                break;
            case SPWPN_PAIN:
                description += "In the hands of one skilled in "
                    "necromantic magic it inflicts "
                    "extra damage on living creatures. ";
                break;
            case SPWPN_DISTORTION:
                description += "It warps and distorts space around it. ";
                break;
            case SPWPN_REACHING:
                description += "It can be evoked to extend its reach. ";
                break;
            case SPWPN_RETURNING:
                description += "It is enchanted to return to its owner "
                               "when thrown.";
                break;
            }
        }

        if (is_random_artefact( item ))
        {
            if (item_ident( item, ISFLAG_KNOW_PROPERTIES ))
            {
                unsigned int old_length = description.length();
                randart_descrip( description, item );

                if (description.length() == old_length)
                    description += "$";
            }
            else if (item_type_known(item))
            {
                description += "$This weapon may have some hidden properties.$";
            }
        }
        else if (spec_ench != SPWPN_NORMAL && item_type_known(item))
        {
            description += "$";
        }
    }

    if (item_known_cursed( item ))
    {
        description += "$It has a curse placed upon it.";
    }

    if (verbose && !is_range_weapon(item))
    {
#ifdef USE_NEW_COMBAT_STATS
        const int str_weight = weapon_str_weight( item.base_type, item.sub_type );

        if (str_weight >= 8)
            description += "$This weapon is best used by the strong.";
        else if (str_weight > 5)
            description += "$This weapon is better for the strong.";
        else if (str_weight <= 2)
            description += "$This weapon is best used by the dexterous.";
        else if (str_weight < 5)
            description += "$This weapon is better for the dexterous.";
#endif

        switch (hands_reqd(item, player_size()))
        {
        case HANDS_ONE:
            description += "$It is a one handed weapon.";
            break;
        case HANDS_HALF:
            description += "$It can be used with one hand, or more "
                    "effectively with two (i.e. when not using a shield).";
            break;
        case HANDS_TWO:
            description += "$It is a two handed weapon.";
            break;
        default:
            description += "$It is a buggy weapon.";
            break;
        }
    }

    if (!is_random_artefact( item ))
    {
        switch (get_equip_race( item ))
        {
        case ISFLAG_DWARVEN:
            description += "$It is well-crafted and very durable.";
            description += "$Dwarves are more deadly with it.";
            break;
        case ISFLAG_ELVEN:
            description += "$Elves are more accurate with it.";
            break;
        case ISFLAG_ORCISH:
            description += "$Orcs are more deadly with it.";
            break;
        }

        if (is_range_weapon(item))
        {
            switch (get_equip_race( item ))
            {
            case ISFLAG_DWARVEN:
                description += "$It is most deadly when used with "
                    "dwarven ammunition.";
                break;
            case ISFLAG_ELVEN:
                description += "$It is most deadly when used with "
                    "elven ammunition.";
                break;
            case ISFLAG_ORCISH:
                description += "$It is most deadly when used with "
                    "orcish ammunition.";
                break;
            }
        }
    }

    if (verbose)
    {
        description += "$It falls into the";

        switch (item.sub_type)
        {
        case WPN_SLING:
            description += " 'slings' category. ";
            break;
        case WPN_BOW:
        case WPN_LONGBOW:
            description += " 'bows' category. ";
            break;
        case WPN_HAND_CROSSBOW:
        case WPN_CROSSBOW:
            description += " 'crossbows' category. ";
            break;
        case WPN_BLOWGUN:
            description += " 'darts' category. ";
            break;
        default:
            // Melee weapons
            switch (weapon_skill(item.base_type, item.sub_type))
            {
            case SK_SHORT_BLADES:
                description += " 'short blades' category. ";
                break;
            case SK_LONG_SWORDS:
                description += " 'long swords' category. ";
                break;
            case SK_AXES:
                description += " 'axes' category. ";
                break;
            case SK_MACES_FLAILS:
                description += " 'maces and flails' category. ";
                break;
            case SK_POLEARMS:
                description += " 'polearms' category. ";
                break;
            case SK_STAVES:
                description += " 'staves' category. ";
                break;
            default:
                description += " 'bug' category. ";
                DEBUGSTR("Unknown weapon type");
                break;
            }
        }
    }

    return (description);
}


//---------------------------------------------------------------
//
// describe_ammo
//
//---------------------------------------------------------------
static std::string describe_ammo( const item_def &item )
{
    std::string description;

    description.reserve(64);

    switch (item.sub_type)
    {
    case MI_STONE:
        description += "A stone. It can be thrown by hand "
            "or fired with a sling. ";
        break;
    case MI_ARROW:
        description += "An arrow, to be shot with a bow. ";
        break;
    case MI_NEEDLE:
        description += "A needle. It can be fired with a blowgun. ";
        break;
    case MI_BOLT:
        description += "A crossbow bolt. ";
        break;
    case MI_DART:
        description += "A small throwing weapon. ";
        break;
    case MI_LARGE_ROCK:
        description += "A rock, used by giants as a missile. ";
        break;
    case MI_SLING_BULLET:
        description += "A small heavy projectile made of lead. "
            "It can be fired with a sling, or thrown by hand.";
        break;
    case MI_JAVELIN:
        description += "A long, light polearm that can be thrown by hand. ";
        if (!is_throwable(item, you.body_size()))
            description += "Unfortunately, it is too long and awkward "
                           "for you to use.";
        break;
    case MI_THROWING_NET:
        description += "A throwing net as used by gladiators. ";
        if (!is_throwable(item, you.body_size()))
            description += "Unfortunately, it is too large and awkward "
                           "for you to use. ";
        if (item.plus < 0)
        {
            std::string how;
            if (item.plus > -3)
                how = "a little";
            else if (item.plus > -5)
                how = "somewhat";
            else if (item.plus > -7)
                how = "very";
            else
                how = "extremely";

            description += "It looks ";
            description += how;
            description += " worn.";
        }
        else if (item.plus > 1)
            description += "The net looks brand-new!";
        break;
    case MI_NONE:   // was eggplant
        description += "A purple vegetable. "
            "The presence of this object in the game indicates a bug. ";
        break;
    default:
        DEBUGSTR("Unknown ammo type");
        break;
    }

    switch ( get_equip_race(item) )
    {
        case ISFLAG_DWARVEN:
            description +=
                "$It is more effective in conjunction with dwarven launchers.";
            break;
        case ISFLAG_ELVEN:
            description +=
                "$It is more effective in conjunction with elven launchers.";
            break;
        case ISFLAG_ORCISH:
            description +=
                "$It is more effective in conjunction with orcish launchers.";
            break;
    }

    if (item.special != 0 && item_type_known(item))
    {
        switch (item.special)
        {
        case 1:
            description += "$When fired from an appropriate launcher, "
                "it turns into a bolt of flame. ";
            break;
        case 2:
            description += "$When fired from an appropriate launcher, "
                "it turns into a bolt of ice. ";
            break;
        case 3:
        case 4:
            description += "$It is coated with poison. ";
            break;
        }
    }

    description += "$";

    return (description);
}


//---------------------------------------------------------------
//
// describe_armour
//
//---------------------------------------------------------------
static std::string describe_armour( const item_def &item, bool verbose )
{
    std::string description;

    description.reserve(200);

    if (is_unrandom_artefact( item )
        && strlen(unrandart_descrip(1, item)) != 0)
    {
        description += unrandart_descrip(1, item);
        description += "$";
    }
    else
    {
        if (verbose)
        {
            switch (item.sub_type)
            {
            case ARM_ROBE:
                description += "A cloth robe. ";
                break;
            case ARM_LEATHER_ARMOUR:
                description += "A suit made of hardened leather. ";
                break;
            case ARM_RING_MAIL:
                description += "A leather suit covered in little rings. ";
                break;
            case ARM_SCALE_MAIL:
                description +=
                    "A leather suit covered in little metal plates. ";
                break;
            case ARM_CHAIN_MAIL:
                description += "A suit made of interlocking metal rings. ";
                break;
            case ARM_SPLINT_MAIL:
                description += "A suit made of splints of metal. ";
                break;
            case ARM_BANDED_MAIL:
                description += "A suit made of bands of metal. ";
                break;
            case ARM_PLATE_MAIL:
                description += "A suit of mail and large plates of metal. ";
                break;
            case ARM_SHIELD:
                description +=
                    "A piece of metal, to be strapped on one's arm. "
                    "It is cumbersome to wear, and slightly slows "
                    "the rate at which you may attack. ";
                break;
            case ARM_CLOAK:
                description += "A cloth cloak. ";
                break;

            case ARM_HELMET:
                switch (get_helmet_type( item ))
                {
                case THELM_HELMET:
                case THELM_HELM:
                    description += "A piece of metal headgear. ";
                    break;
                case THELM_CAP:
                    description += "A cloth or leather cap. ";
                    break;
                case THELM_WIZARD_HAT:
                    description += "A conical cloth hat. ";
                    break;
                }
                break;

            case ARM_GLOVES:
                description += "A pair of gloves. ";
                break;

            case ARM_CENTAUR_BARDING:
                description += "An armour made for centaurs, "
                               "to wear over their equine half.";
                break;

            case ARM_NAGA_BARDING:
                description += "A special armour made for nagas, "
                               "to wear over their tails.";
                break;

            case ARM_BOOTS:
                description += "A pair of boots.";
                break;

            case ARM_BUCKLER:
                description += "A small shield. ";
                break;
            case ARM_LARGE_SHIELD:
                description += "Like a normal shield, only larger. ";
                if (you.species == SP_TROLL || you.species == SP_OGRE
                    || you.species == SP_OGRE_MAGE
                    || player_genus(GENPC_DRACONIAN))
                {
                    description += "It looks like it would fit you well. ";
                }
                else
                {
                    description += "It is very cumbersome to wear, and "
                        "slows the rate at which you may attack. ";
                }
                break;
            case ARM_DRAGON_HIDE:
                description += "The scaly skin of a dragon. I suppose "
                    "you could wear it if you really wanted to. ";
                break;
            case ARM_TROLL_HIDE:
                description += "The stiff and knobbly hide of a troll. "
                    "I suppose you could wear it "
                    "if you really wanted to. ";
                break;
            case ARM_CRYSTAL_PLATE_MAIL:
                description += "An incredibly heavy but extremely effective "
                    "suit of crystalline armour. "
                    "It is somewhat resistant to corrosion. ";
                break;
            case ARM_DRAGON_ARMOUR:
                description += "A magical armour, made from the scales of "
                    "a fire-breathing dragon. It provides "
                    "great protection from the effects of fire, "
                    "but renders its wearer more susceptible to "
                    "the effects of cold. ";
                break;
            case ARM_TROLL_LEATHER_ARMOUR:
                description += "A magical armour, made from the stiff and "
                    "knobbly skin of a common troll. It magically regenerates "
                    "its wearer's flesh at a fairly slow rate "
                    "(unless already a troll). ";
                break;
            case ARM_ICE_DRAGON_HIDE:
                description += "The scaly skin of a dragon. I suppose "
                    "you could wear it if you really wanted to. ";
                break;
            case ARM_ICE_DRAGON_ARMOUR:
                description += "A magical armour, made from the scales of "
                    "a cold-breathing dragon. It provides "
                    "great protection from the effects of cold, "
                    "but renders its wearer more susceptible to "
                    "the effects of fire and heat. ";
                break;
            case ARM_STEAM_DRAGON_HIDE:
                description += "The soft and supple scaly skin of "
                    "a steam dragon. I suppose you could "
                    "wear it if you really wanted to. ";
                break;
            case ARM_STEAM_DRAGON_ARMOUR:
                description += "A magical armour, made from the scales of "
                    "a steam-breathing dragon. Although unlike "
                    "the armour made from the scales of some "
                    "larger dragons it does not provide its wearer "
                    "with much in the way of special magical "
                    "protection, it is extremely light and "
                    "as supple as cloth. ";
                break;          /* Protects from steam */
            case ARM_MOTTLED_DRAGON_HIDE:
                description += "The weirdly-patterned scaley skin of "
                    "a mottled dragon. I suppose you could "
                    "wear it if you really wanted to. ";
                break;
            case ARM_MOTTLED_DRAGON_ARMOUR:
                description += "A magical armour made from the scales of a "
                    "mottled dragon. Although unlike the armour "
                    "made from the scales of some larger dragons "
                    "it does not provide its wearer with much in "
                    "the way of special magical protection, it is "
                    "as light and relatively uncumbersome as "
                    "leather armour. ";
                break;          /* Protects from napalm */
            case ARM_STORM_DRAGON_HIDE:
                description += "The hide of a storm dragon, covered in "
                    "extremely hard blue scales. I suppose "
                    "you could wear it if you really wanted to. ";
                break;
            case ARM_STORM_DRAGON_ARMOUR:
                description += "A magical armour made from the scales of "
                    "a lightning-breathing dragon. It is heavier "
                    "than most dragon scale armours, but gives "
                    "its wearer great resistance to "
                    "electrical discharges. ";
                break;
            case ARM_GOLD_DRAGON_HIDE:
                description += "The extremely tough and heavy skin of a "
                    "golden dragon, covered in shimmering golden "
                    "scales. I suppose you could wear it if "
                    "you really wanted to. ";
                break;
            case ARM_GOLD_DRAGON_ARMOUR:
                description += "A magical armour made from the golden scales "
                    "of a golden dragon. It is extremely heavy and "
                    "cumbersome, but confers resistances to fire, "
                    "cold, and poison on its wearer. ";
                break;
            case ARM_ANIMAL_SKIN:
                description += "The skins of several animals. ";
                break;
            case ARM_SWAMP_DRAGON_HIDE:
                description += "The slimy";
                if (player_can_smell())
                    description += ", smelly";
                description += " skin of a swamp-dwelling dragon. I suppose "
                    "you could wear it if you really wanted to. ";
                break;
            case ARM_SWAMP_DRAGON_ARMOUR:
                description += "A magical armour made from the scales of "
                    "a swamp dragon. It confers resistance to "
                    "poison on its wearer. ";
                break;
            default:
                DEBUGSTR("Unknown armour");
            }

            description += "$";
        }
    }

    if (verbose 
            && item.sub_type != ARM_SHIELD 
            && item.sub_type != ARM_BUCKLER
            && item.sub_type != ARM_LARGE_SHIELD)
    {
        description += "$Armour rating: ";

        if (item.sub_type == ARM_HELMET 
            && (get_helmet_type( item ) == THELM_CAP 
                || get_helmet_type( item ) == THELM_WIZARD_HAT))
        {
            // caps and wizard hats don't have a base AC
            append_value(description, 0, false);
        }
        else if (item.sub_type == ARM_NAGA_BARDING 
                || item.sub_type == ARM_CENTAUR_BARDING)
        {
            // Barding has AC value 4.
            append_value(description, 4, false);
        }
        else
        {
            append_value(description, property( item, PARM_AC ), false);
        }

        description += "$Evasion modifier: ";
        append_value(description, property( item, PARM_EVASION ), true);
        description += "$";
    }

    const int ego = get_armour_ego_type( item );
    if (ego != SPARM_NORMAL 
        && item_type_known(item) 
        && verbose)
    {
        description += "$";

        switch (ego)
        {
        case SPARM_RUNNING:
            description += "It allows its wearer to run at a great speed. ";
            break;
        case SPARM_FIRE_RESISTANCE:
            description += "It protects its wearer from heat and fire. ";
            break;
        case SPARM_COLD_RESISTANCE:
            description += "It protects its wearer from cold. ";
            break;
        case SPARM_POISON_RESISTANCE:
            description += "It protects its wearer from poison. ";
            break;
        case SPARM_SEE_INVISIBLE:
            description += "It allows its wearer to see invisible things. ";
            break;
        case SPARM_DARKNESS:
            description += "When activated it hides its wearer from "
                "the sight of others, but also increases "
                "their metabolic rate by a large amount. ";
            break;
        case SPARM_STRENGTH:
            description += "It increases the physical power of its wearer (+3 to strength). ";
            break;
        case SPARM_DEXTERITY:
            description += "It increases the dexterity of its wearer (+3 to dexterity). ";
            break;
        case SPARM_INTELLIGENCE:
            description += "It makes you more clever (+3 to intelligence). ";
            break;
        case SPARM_PONDEROUSNESS:
            description += "It is very cumbersome (-2 to EV, slows movement). ";
            break;
        case SPARM_LEVITATION:
            description += "It can be activated to allow its wearer to "
                "float above the ground and remain so indefinitely. ";
            break;
        case SPARM_MAGIC_RESISTANCE:
            description += "It increases its wearer's resistance "
                "to enchantments. ";
            break;
        case SPARM_PROTECTION:
            description += "It protects its wearer from harm (+3 to AC). ";
            break;
        case SPARM_STEALTH:
            description += "It enhances the stealth of its wearer. ";
            break;
        case SPARM_RESISTANCE:
            description += "It protects its wearer from the effects "
                "of both cold and heat. ";
            break;

        // these two are robes only:
        case SPARM_POSITIVE_ENERGY:
            description += "It partially protects its wearer from "
                "the effects of negative energy. ";
            break;
        case SPARM_ARCHMAGI:
            description += "It greatly increases the power of its "
                "wearer's magical spells, but is only "
                "intended for those who have " "very little left to learn. ";
            break;

        case SPARM_PRESERVATION:
            description += "It protects its wearer's possessions "
                "from damage and destruction. ";
            break;
        }

        description += "$";
    }

    if (is_random_artefact( item ))
    {
        if (item_ident( item, ISFLAG_KNOW_PROPERTIES ))
            randart_descrip( description, item );
        else if (item_type_known(item))
            description += "$This armour may have some hidden properties.$";
    }
    else 
    {
        switch (get_equip_race( item ))  
        {
        case ISFLAG_ELVEN:
            description += "$It is well-crafted and unobstructive";

            if (item.sub_type == ARM_CLOAK || item.sub_type == ARM_BOOTS)
                description += ", and helps its wearer avoid being noticed";

            description += ".";
            description += "$It fits elves well.";
            break;

        case ISFLAG_DWARVEN:
            description += "$It is well-crafted and very durable.";
            description += "$It fits dwarves well.";
            break;
        
        case ISFLAG_ORCISH:
            description += "$It fits orcs well.";
            break;

        default:
            break;
        }
    }

    if (verbose && get_armour_slot(item) == EQ_BODY_ARMOUR)
    {
        if ( is_light_armour(item) )
            description += "$This is a light armour. Wearing it will "
                "exercise Dodging and Stealth.";
        else
            description += "$This is a heavy armour. Wearing it will "
                "exercise Armour.";
    }

    if (item_known_cursed( item ))
    {
        description += "$It has a curse placed upon it.";
    }

    return description;
}

//---------------------------------------------------------------
//
// describe_stick
//
//---------------------------------------------------------------
static std::string describe_stick( const item_def &item )
{
    std::string description;

    description.reserve(64);

    if ( !item_type_known(item) )
        description += "A stick. Maybe it's magical. ";
    else
    {
        description += "A magical device which ";
        switch (item.sub_type)
        {
        case WAND_FLAME:
            description += "throws little bits of flame. ";
            break;

        case WAND_FROST:
            description += "throws little bits of frost. ";
            break;

        case WAND_SLOWING:
            description += "casts enchantments to slow down the actions of "
                "a creature at which it is directed. ";
            break;

        case WAND_HASTING:
            description += "casts enchantments to speed up the actions of "
                "a creature at which it is directed. ";
            break;

        case WAND_MAGIC_DARTS:
            description += "throws small bolts of destructive energy. ";
            break;

        case WAND_HEALING:
            description += "can heal a creature's wounds. ";
            break;

        case WAND_PARALYSIS:
            description += "can render a creature immobile. ";
            break;

        case WAND_FIRE:
            description += "throws great bolts of fire. ";
            break;

        case WAND_COLD:
            description += "throws great bolts of cold. ";
            break;

        case WAND_CONFUSION:
            description += "induces confusion and bewilderment in "
                "a target creature. ";
            break;

        case WAND_INVISIBILITY:
            description += "hides a creature from the view of others. ";
            break;

        case WAND_DIGGING:
            description += "drills tunnels through unworked rock. ";
            break;

        case WAND_FIREBALL:
            description += "throws exploding blasts of flame. ";
            break;

        case WAND_TELEPORTATION:
            description += "causes a creature to be randomly translocated. ";
            break;

        case WAND_LIGHTNING:
            description += "throws great bolts of lightning. ";
            break;

        case WAND_POLYMORPH_OTHER:
            description += "causes a creature to be transmogrified into "
                "another form. "
                "It doesn't work on you, so don't even try. ";
            break;

        case WAND_ENSLAVEMENT:
            description += "causes slavish obedience in a creature. ";
            break;

        case WAND_DRAINING:
            description += "throws a bolt of negative energy which "
                "drains the life essences of living creatures, "
                "but is useless against the undead. ";
            break;

        case WAND_RANDOM_EFFECTS:
            description += "can produce a variety of effects. ";
            break;

        case WAND_DISINTEGRATION:
            description += "disrupts the physical structure of "
                "anything but the hardest walls -- even rigid "
                "statues, to say nothing of flesh. ";
            break;

        default:
            DEBUGSTR("Unknown stick");
        }

        if (item_ident( item, ISFLAG_KNOW_PLUSES ) && item.plus == 0)
            description += "Unfortunately, it has no charges left. ";
    }

    return description;
}


//---------------------------------------------------------------
//
// describe_food
//
//---------------------------------------------------------------
static std::string describe_food( const item_def &item )
{
    std::string description;

    description.reserve(100);

    switch (item.sub_type)
    {
    // rations
    case FOOD_MEAT_RATION:
    case FOOD_BREAD_RATION:
        description += "A filling ration of ";
        switch (item.sub_type)
        {
        case FOOD_MEAT_RATION:
            description += "dried and preserved meats";
            break;
        case FOOD_BREAD_RATION:
            description += "breads";
            break;
        }
        description += ". ";
        break;

    // fruits
    case FOOD_PEAR:
    case FOOD_APPLE:
    case FOOD_APRICOT:
    case FOOD_ORANGE:
    case FOOD_BANANA:
    case FOOD_STRAWBERRY:
    case FOOD_RAMBUTAN:
    case FOOD_LEMON:
    case FOOD_GRAPE:
    case FOOD_LYCHEE:
    case FOOD_SULTANA:
        description += "A";
        switch (item.sub_type)
        {
        case FOOD_PEAR:
            description += " delicious juicy";
            break;
        case FOOD_APPLE:
            description += " delicious red or green";
            break;
        case FOOD_APRICOT:
            description += " delicious orange";
            break;
        case FOOD_ORANGE:
            description += " delicious juicy orange";
            break;
        case FOOD_BANANA:
            description += " delicious yellow";
            break;
        case FOOD_STRAWBERRY:
            description += " small but delicious red";
            break;
        case FOOD_RAMBUTAN:
            description += " small but delicious tropical";
            break;
        case FOOD_LEMON:
            description += " yellow";
            break;
        case FOOD_GRAPE:
            description += " small";
            break;
        case FOOD_LYCHEE:
            description += " tropical";
            break;
        case FOOD_SULTANA:
            description += " dried";
            break;
        }

        description += " fruit";

        switch (item.sub_type)
        {
        case FOOD_BANANA:
            description += ", probably grown and imported by "
                "some amoral multinational as the "
                "result of a corrupt trade deal";
            break;
        case FOOD_RAMBUTAN:
            description += ". How it got into this dungeon "
                "is anyone's guess";
            break;
        case FOOD_SULTANA:
            description += " of some sort, possibly a grape";
            break;
        }
        description += ". ";
        break;

    // vegetables
    case FOOD_CHOKO:
    case FOOD_SNOZZCUMBER:
        description += "A";
        switch (item.sub_type)
        {
        case FOOD_CHOKO:
            description += "n almost tasteless green";
            break;
        case FOOD_SNOZZCUMBER:
            description += " repulsive cucumber-shaped";
            break;
        }
        description += " vegetable";
        switch (item.sub_type)
        {
        case FOOD_CHOKO:
            description += ", which grows on a vine";
            break;
        }
        description += ". ";
        break;

    // lumps, slices, chunks, and strips
    case FOOD_HONEYCOMB:
    case FOOD_ROYAL_JELLY:
    case FOOD_PIZZA:
    case FOOD_CHEESE:
    case FOOD_BEEF_JERKY:
    case FOOD_SAUSAGE:
    case FOOD_CHUNK:
        description += "A";
        switch (item.sub_type)
        {
        case FOOD_SAUSAGE:
            description += "n elongated";
            break;
        }
        switch (item.sub_type)
        {
        case FOOD_HONEYCOMB:
        case FOOD_ROYAL_JELLY:
        case FOOD_CHEESE:
        case FOOD_SAUSAGE:
            description += " lump";
            break;
        case FOOD_PIZZA:
            description += " slice";
            break;
        case FOOD_BEEF_JERKY:
            description += " strip";
            break;
        case FOOD_CHUNK:
            description += " piece";
        }
        description += " of ";
        switch (item.sub_type)
        {
        case FOOD_SAUSAGE:
            description += "low-grade gristle, entrails and "
                "cereal products encased in an intestine";
            break;
        case FOOD_HONEYCOMB:
            description += "the delicious honeycomb made by giant bees";
            break;
        case FOOD_ROYAL_JELLY:
            description += "the magical substance produced by giant bees "
                "to be fed to their queens";
            break;
        case FOOD_PIZZA:
            description += "pizza";
            break;
        case FOOD_CHEESE:
            description += "cheese";
            break;
        case FOOD_BEEF_JERKY:
            description += "preserved dead cow or bull";
            break;
        case FOOD_CHUNK:
            description += "dungeon meat";
            break;
        }
        description += ". ";
        switch (item.sub_type)
        {
        case FOOD_SAUSAGE:
            description += "Yum! ";
            break;
        case FOOD_PIZZA:
            description += "Don't tell me you don't know what that is! ";
            break;
        case FOOD_CHUNK:
            if (you.species != SP_GHOUL)
                description += "It looks rather unpleasant. ";

            if (item.special < 100)
            {
                if (you.species == SP_GHOUL)
                    description += "It looks nice and ripe. ";
                else if (you.species != SP_MUMMY)
                {
                    description += "In fact, it is "
                        "rotting away before your eyes. "
                        "Eating it would probably be unwise. ";
                }
            }
            break;
        }
        break;

    default:
        DEBUGSTR("Unknown food");
    }

    description += "$";

    return (description);
}

//---------------------------------------------------------------
//
// describe_potion
//
//---------------------------------------------------------------
static const char* describe_potion( const item_def &item )
{
    if ( !item_type_known(item) )
        return "A small bottle of liquid.$";

    switch (static_cast<potion_type>(item.sub_type))
    {
    case POT_HEALING:
        return "A blessed fluid which heals some wounds, clears the mind, "
            "and cures diseases. If one uses it when they are at or near "
            "full health, it can also slightly repair permanent injuries.$";
    case POT_HEAL_WOUNDS:
        return "A magical healing elixir which causes wounds to close and "
            "heal almost instantly. If one uses it when they are at or near "
            "full health, it can also repair permanent injuries.$";
    case POT_SPEED:
        return "An enchanted beverage which speeds the actions of anyone who "
            "drinks it.$";
    case POT_MIGHT:
        return "A magic potion which greatly increases the strength and "
            "physical power of one who drinks it.$";
    case POT_GAIN_STRENGTH:
        return "A potion of beneficial mutation.$";
    case POT_GAIN_DEXTERITY:
        return "A potion of beneficial mutation.$";
    case POT_GAIN_INTELLIGENCE:
        return "A potion of beneficial mutation.$";
    case POT_LEVITATION:
        return "A potion which confers great buoyancy "
            "on one who consumes it.$";
    case POT_POISON:
        return "A nasty poisonous liquid.$";
    case POT_SLOWING:
        return "A potion which slows your actions.$";
    case POT_PARALYSIS:
        return "A potion which eliminates your control over your own body.$";
    case POT_CONFUSION:
        return "A potion which confuses your perceptions and "
            "reduces your control over your own actions.$";
    case POT_INVISIBILITY:
        return "A potion which hides you from the sight of others.$";
    case POT_PORRIDGE:
        return "A filling potion of sludge, high in cereal fibre.$";
    case POT_DEGENERATION:
        return "A noxious concoction which can do terrible things "
            "to your body, brain and reflexes.$";
    case POT_DECAY:
        return "A vile and putrid cursed liquid which causes your "
            "flesh to decay before your very eyes.$";
    case POT_WATER:
        return "A unique substance, vital for the existence of most life.$";
    case POT_EXPERIENCE:
        return "A truly wonderful and very rare drink.$";
    case POT_MAGIC:
        return "A valuable potion which grants a person with an "
            "infusion of magical energy.$";
    case POT_RESTORE_ABILITIES:
        return "A potion which restores the abilities of one who drinks it.$";
    case POT_STRONG_POISON:
        return "A terribly venomous potion.$";
    case POT_BERSERK_RAGE:
        return "A potion which can send one into an incoherent rage.$";
    case POT_CURE_MUTATION:
        return "A potion which removes some or all of any mutations "
            "which may be afflicting you.$";
    case POT_MUTATION:
        return "A potion which does very strange things to you.$";
    case POT_BLOOD:
        return "A potion containing the essence of life. Vital for all living "
               "creatures, as well as some undead ones.$";
    case POT_RESISTANCE:
        return "A potion which grants you temporary resistance to the elements "
               "and poison.$";
    case NUM_POTIONS:
        return "A buggy potion.";
    }
    return "A very buggy potion.";
}


//---------------------------------------------------------------
//
// describe_scroll
//
//---------------------------------------------------------------
static std::string describe_scroll( const item_def &item )
{
    std::string description;

    description.reserve(64);

    if ( !item_type_known(item) )
        description += "A scroll of paper covered in magical writing.";
    else
    {
        switch (item.sub_type)
        {
        case SCR_IDENTIFY:
            description += "This useful magic scroll allows you to "
                "determine the properties of any object. ";
            break;

        case SCR_TELEPORTATION:
            description += "Reading the words on this scroll "
                "translocates you to a random position. ";
            break;

        case SCR_FEAR:
            description += "This scroll causes great fear in those "
                "who see the one who reads it. ";
            break;

        case SCR_NOISE:
            description += "This prank scroll, often slipped into a wizard's "
                "backpack by a devious apprentice, causes a loud noise. "
                "It is not otherwise noted for its usefulness. ";
            break;

        case SCR_REMOVE_CURSE:
            description += "Reading this scroll removes curses from "
                "the items you are using. ";
            break;

        case SCR_DETECT_CURSE:
            description += "This scroll allows you to detect the presence "
                "of cursed items among your possessions. ";
            break;

        case SCR_SUMMONING:
            description += "This scroll opens a conduit to the Abyss "
                "and draws a terrible beast to this world "
                "for a limited time. ";
            break;

        case SCR_ENCHANT_WEAPON_I:
            description += "This scroll places an enchantment on a weapon, "
                "making it more accurate in combat. It may fail "
                "to affect weapons already heavily enchanted. ";
            break;

        case SCR_ENCHANT_ARMOUR:
            description += "This scroll places an enchantment "
                "on a piece of armour. ";
            break;

        case SCR_TORMENT:
            description += "This scroll calls on the powers of Hell to "
                "inflict great pain on any nearby creature - "
                "including you! ";
            break;

        case SCR_RANDOM_USELESSNESS:
            description += "It is easy to be blinded to the essential "
                "uselessness of this scroll by the sense of achievement "
                "you get from getting it to work at all.";
                // -- The Hitchhiker's Guide to the Galaxy (paraphrase)
            break;

        case SCR_CURSE_WEAPON:
            description += "This scroll places a curse on a weapon. ";
            break;

        case SCR_CURSE_ARMOUR:
            description += "This scroll places a curse "
                "on a piece of armour. ";
            break;

        case SCR_IMMOLATION:
            description += "Small writing on the back of the scroll reads: "
                "\"Warning: contents under pressure.  Do not use near"
                " flammable objects.\"";
            break;

        case SCR_BLINKING:
            description += "This scroll allows its reader to teleport "
                "a short distance, with precise control.  Be wary that "
                "controlled teleports will cause the subject to "
                "become contaminated with magical energy. ";
            break;

        case SCR_PAPER:
            description += "Apart from a label, this scroll is blank. ";
            break;

        case SCR_MAGIC_MAPPING:
            description += "This scroll reveals the nearby surroundings "
                "of one who reads it. ";
            break;

        case SCR_FORGETFULNESS:
            description += "This scroll induces "
                "an irritating disorientation. ";
            break;

        case SCR_ACQUIREMENT:
            description += "This wonderful scroll causes the "
                "creation of a valuable item to "
                "appear before the reader. "
                "It is especially treasured by specialist "
                "magicians, as they can use it to obtain "
                "the powerful spells of their specialty. ";
            break;

        case SCR_ENCHANT_WEAPON_II:
            description += "This scroll places an enchantment on a weapon, "
                "making it inflict greater damage in combat. "
                "It may fail to affect weapons already "
                "heavily enchanted. ";
            break;

        case SCR_VORPALISE_WEAPON:
            description += "This scroll enchants a weapon so as to make "
                "it far more effective at inflicting harm on "
                "its wielder's enemies. Using it on a weapon "
                "already affected by some kind of special "
                "enchantment (other than that produced by a "
                "normal scroll of enchant weapon) is not advised. ";
            break;

        case SCR_RECHARGING:
            description += "This scroll restores the charges of "
                "any magical wand wielded by its reader. ";
            break;

        case SCR_ENCHANT_WEAPON_III:
            description += "This scroll enchants a weapon to be "
                "far more effective in combat. Although "
                "it can be used in the creation of especially "
                "enchanted weapons, it may fail to affect those "
                "already heavily enchanted. ";
            break;

        default:
            DEBUGSTR("Unknown scroll");
        }
    }

    description += "$";

    return (description);
}


//---------------------------------------------------------------
//
// describe_jewellery
//
//---------------------------------------------------------------
static std::string describe_jewellery( const item_def &item, bool verbose)
{
    std::string description;

    description.reserve(200);

    if (is_unrandom_artefact( item ) && strlen(unrandart_descrip(1, item)) != 0)
    {
        description += unrandart_descrip(1, item);
        description += "$";
    }
    else if ( !item_type_known(item) )
    {
        description += "A piece of jewellery.";
    }
    else if (verbose || is_random_artefact( item ))
    {
        switch (item.sub_type)
        {
        case RING_REGENERATION:
            description += "This wonderful ring greatly increases the "
                "recuperative powers of its wearer, but also "
                "considerably speeds his or her metabolism. ";
            break;

        case RING_PROTECTION:
            description +=
                "This ring either protects its wearer from harm or makes "
                "them more vulnerable to injury, to a degree dependent "
                "on its power. ";
            break;

        case RING_PROTECTION_FROM_FIRE:
            description +=
                "This ring provides protection from heat and fire. ";
            break;

        case RING_POISON_RESISTANCE:
            description +=
                "This ring provides protection from the effects of poisons and venom. ";
            break;

        case RING_PROTECTION_FROM_COLD:
            description += "This ring provides protection from cold. ";
            break;

        case RING_STRENGTH:
            description +=
                "This ring increases or decreases the physical strength "
                "of its wearer, to a degree dependent on its power. ";
            break;

        case RING_SLAYING:
            description +=
                "This ring increases the hand-to-hand and missile combat "
                "skills of its wearer.";
            break;

        case RING_SEE_INVISIBLE:
            description +=
                "This ring allows its wearer to see those things hidden "
                "from view by magic. ";
            break;

        case RING_INVISIBILITY:
            description +=
                "This powerful ring can be activated to hide its wearer "
                "from the view of others, but increases the speed of his "
                "or her metabolism greatly while doing so. ";
            break;

        case RING_HUNGER:
            description +=
                "This accursed ring causes its wearer to hunger "
                "considerably more quickly. ";
            break;

        case RING_TELEPORTATION:
            description +=
                "This ring occasionally exerts its power to randomly "
                "translocate its wearer to another place, and can be "
                "deliberately activated for the same effect. ";
            break;

        case RING_EVASION:
            description +=
                "This ring makes its wearer either more or less capable "
                "of avoiding attacks, depending on its degree "
                "of enchantment. ";
            break;

        case RING_SUSTAIN_ABILITIES:
            description +=
                "This ring protects its wearer from the loss of their "
                "strength, dexterity and intelligence. ";
            break;

        case RING_SUSTENANCE:
            description +=
                "This ring provides energy to its wearer, so that they "
                "need eat less often. ";
            break;

        case RING_DEXTERITY:
            description +=
                "This ring increases or decreases the dexterity of its "
                "wearer, depending on the degree to which it has been "
                "enchanted. ";
            break;

        case RING_INTELLIGENCE:
            description +=
                "This ring increases or decreases the mental ability of "
                "its wearer, depending on the degree to which it has "
                "been enchanted. ";
            break;

        case RING_WIZARDRY:
            description +=
                "This ring increases the ability of its wearer to use "
                "magical spells. ";
            break;

        case RING_MAGICAL_POWER:
            description +=
                "This ring increases its wearer's reserves of magical "
                "power. ";
            break;

        case RING_LEVITATION:
            description +=
                "This ring allows its wearer to hover above the floor. ";
            break;

        case RING_LIFE_PROTECTION:
            description +=
                "This blessed ring protects the life-force of its wearer "
                "from negative energy, making them partially immune to "
                "the draining effects of undead and necromantic magic. ";
            break;

        case RING_PROTECTION_FROM_MAGIC:
            description +=
                "This ring increases its wearer's resistance to "
                "hostile enchantments. ";
            break;

        case RING_FIRE:
            description +=
                "This ring brings its wearer more in contact with "
                "the powers of fire. He or she gains resistance to "
                "heat and can use fire magic more effectively, but "
                "becomes more vulnerable to the effects of cold. ";
            break;

        case RING_ICE:
            description +=
                "This ring brings its wearer more in contact with "
                "the powers of cold and ice. He or she gains resistance "
                "to cold and can use ice magic more effectively, but "
                "becomes more vulnerable to the effects of fire. ";
            break;

        case RING_TELEPORT_CONTROL:
            description += "This ring allows its wearer to control the "
                "destination of any teleportation, although without "
                "perfect accuracy.  Trying to teleport into a solid "
                "object will result in a random teleportation, at "
                "least in the case of a normal teleportation.  Also "
                "be wary that controlled teleports will contaminate "
                "the subject with residual magical energy.";
            break;

        case AMU_RAGE:
            description +=
                "This amulet enables its wearer to attempt to enter "
                "a state of berserk rage, and increases their chance "
                "of successfully doing so.  It also partially protects "
                "the user from passing out when coming out of that rage. ";
            break;

        case AMU_RESIST_SLOW:
            description +=
                "This amulet protects its wearer from some magically "
                "induced forms of slowness, and increases the duration "
                "of enchantments which speed his or her actions. ";
            break;

        case AMU_CLARITY:
            description +=
                "This amulet protects its wearer from some forms of "
                "mental confusion. ";
            break;

        case AMU_WARDING:
            description +=
                "This amulet repels some of the attacks of creatures "
                "which have been magically summoned, and also "
                "makes the wearer more resistant to draining attacks. ";
            break;

        case AMU_RESIST_CORROSION:
            description +=
                "This amulet protects the wearer and their equipment "
                "from corrosion caused by acids, although not "
                "infallibly so. ";
            break;

        case AMU_THE_GOURMAND:
            description +=
                "This amulet protects its wearer from "
                "sickness from eating fresh raw meat and allows them to "
                "digest it when not hungry, but its effects on the wearer's "
                "digestion are cumulative over time, and are initially "
                "small.";
            break;

        case AMU_CONSERVATION:
            description +=
                "This amulet protects some of the possessions of "
                "its wearer from outright destruction, but not "
                "infallibly so. ";
            break;

        case AMU_CONTROLLED_FLIGHT:
            description +=
                "Should the wearer of this amulet be levitated "
                "by magical means, he or she will be able to exercise "
                "some control over the resulting motion. This allows "
                "the descent of staircases and the retrieval of items "
                "lying on the ground, for example, but does not "
                "deprive the wearer of the benefits of levitation. ";
            break;

        case AMU_INACCURACY:
            description +=
                "This amulet makes its wearer less accurate in hand combat. ";
            break;

        case AMU_RESIST_MUTATION:
            description +=
                "This amulet protects its wearer from mutations, "
                "although not infallibly so. ";
            break;

        default:
            DEBUGSTR("Unknown jewellery");
        }

        description += "$";
    }

    if ((verbose || is_random_artefact( item ))
        && item_ident( item, ISFLAG_KNOW_PLUSES ))
    {
        // Explicit description of ring power (useful for randarts)
        // Note that for randarts we'll print out the pluses even
        // in the case that its zero, just to avoid confusion. -- bwr
        if (item.plus != 0 
            || (item.sub_type == RING_SLAYING && item.plus2 != 0)
            || is_random_artefact( item ))
        {
            switch (item.sub_type)
            {
            case RING_PROTECTION:
                description += "$It affects your AC (";
                append_value( description, item.plus, true );
                description += ").";
                break;

            case RING_EVASION:
                description += "$It affects your evasion (";
                append_value( description, item.plus, true );
                description += ").";
                break;

            case RING_STRENGTH:
                description += "$It affects your strength (";
                append_value( description, item.plus, true );
                description += ").";
                break;

            case RING_INTELLIGENCE:
                description += "$It affects your intelligence (";
                append_value( description, item.plus, true );
                description += ").";
                break;

            case RING_DEXTERITY:
                description += "$It affects your dexterity (";
                append_value( description, item.plus, true );
                description += ").";
                break;

            case RING_SLAYING:
                if (item.plus != 0 || is_random_artefact( item ))
                {
                    description += "$It affects your accuracy (";
                    append_value( description, item.plus, true );
                    description += ").";
                }

                if (item.plus2 != 0 || is_random_artefact( item ))
                {
                    description += "$It affects your damage-dealing abilities (";
                    append_value( description, item.plus2, true );
                    description += ").";
                }
                break;
            
            default:
                break;
            }
        }
    }

    // randart properties
    if (is_random_artefact( item ))
    {
        if (item_ident( item, ISFLAG_KNOW_PROPERTIES ))
            randart_descrip( description, item );
        else if (item_type_known(item))
        {
            if (item.sub_type >= AMU_RAGE)
                description += "$This amulet may have hidden properties.$";
            else
                description += "$This ring may have hidden properties.$";
        }
    }

    if (item_known_cursed( item ))
    {
        description += "$It has a curse placed upon it.";
    }

    return (description);
}                               // end describe_jewellery()

//---------------------------------------------------------------
//
// describe_staff
//
//---------------------------------------------------------------
static std::string describe_staff( const item_def &item )
{
    std::string description;

    description.reserve(200);

    if (item_type_known(item))
    {
        // NB: the leading space is here {dlb}
        description += "This " + std::string( item_is_staff( item ) ? "staff " 
                                                                    : "rod " );

        switch (item.sub_type)
        {
        case STAFF_WIZARDRY:
            description +=
                "significantly increases the ability of its wielder to use "
                "magical spells. ";
            break;

        case STAFF_POWER:
            description +=
                "provides a reservoir of magical power to its wielder. ";
            break;

        case STAFF_FIRE:
            description +=
                "increases the power of fire spells cast by its wielder, "
                "and protects him or her from the effects of heat and fire. "
                "It can burn those struck by it. ";
            break;

        case STAFF_COLD:
            description +=
                "increases the power of ice spells cast by its wielder, "
                "and protects him or her from the effects of cold. It can "
                "freeze those struck by it. ";
            break;

        case STAFF_POISON:
            description +=
                "increases the power of poisoning spells cast by its "
                "wielder, and protects him or her from the effects of "
                "poison. It can poison those struck by it. ";
            break;

        case STAFF_ENERGY:
            description +=
                "allows its wielder to cast magical spells without "
                "hungering as a result. ";
            break;

        case STAFF_DEATH:
            description +=
                "increases the power of necromantic spells cast by its "
                "wielder. It can cause great pain in those living souls "
                "its wielder strikes. ";
            break;

        case STAFF_CONJURATION:
            description +=
                "increases the power of conjurations cast by its wielder. ";
            break;

        case STAFF_ENCHANTMENT:
            description +=
                "increases the power of enchantments cast by its wielder. ";
            break;

        case STAFF_SUMMONING:
            description +=
                "increases the power of summonings cast by its wielder. ";
            break;

        case STAFF_SMITING:
            description +=
                "allows its wielder to smite foes from afar. The wielder "
                "must be at least level four to safely use this ability, "
                "which drains four charges. ";
            break;

        case STAFF_VENOM:
            description +=
                "contains offensive and defensive spells of poison.";
            break;

        case STAFF_STRIKING:
            description += "allows its wielder to strike foes from afar "
                "with force bolts. ";
            break;

        case STAFF_SPELL_SUMMONING:
            description += "contains spells of summoning. ";
            break;

        case STAFF_WARDING:
            description +=
                "contains spells designed to repel one's enemies. ";
            break;

        case STAFF_DISCOVERY:
            description +=
                "contains spells which reveal various aspects of "
                "an explorer's surroundings to them. ";
            break;

        case STAFF_AIR:
            description +=
                "increases the power of air spells cast by its wielder. "
                "It can shock those struck by it. ";
            break;

        case STAFF_EARTH:
            description +=
                "increases the power of earth spells cast by its wielder. "
                "It can crush those struck by it. ";
            break;

        case STAFF_CHANNELING:
            description +=
                "allows its caster to channel ambient magical energy for "
                "his or her own purposes. ";
            break;

        default:
            description +=
                "contains spells of mayhem and destruction. ";
            break;
        }

        if (item_is_rod( item ))
        {
            description += 
                "$$It uses its own mana reservoir for casting spells, and "
                "recharges automatically by channeling mana from its "
                "wielder.";
        }
        else
        {
            description +=
                "$$Damage rating: 7 $Accuracy rating: +6 $Attack delay: 120%";

            description += "$$It falls into the 'staves' category. ";
        }
    }
    else
    {
        description += "A stick imbued with magical properties.$";
    }

    return (description);
}


//---------------------------------------------------------------
//
// describe_misc_item
//
//---------------------------------------------------------------
static std::string describe_misc_item( const item_def &item )
{
    std::string description;

    description.reserve(100);

    const misc_item_type subtype = static_cast<misc_item_type>(item.sub_type);

    if (item_type_known(item))
    {
        switch (subtype)
        {
        case MISC_BOTTLED_EFREET:
            description +=
                "A mighty efreet, captured by some wizard and bound into "
                "a bronze flask. Breaking the flask's seal will release it "
                "to wreak havoc - possibly on you. ";
            break;
        case MISC_CRYSTAL_BALL_OF_SEEING:
            description +=
                "A magical device which allows one to see the layout of "
                "their surroundings. It requires a degree of magical "
                "ability to be used reliably, otherwise it can produce "
                "unpredictable and possibly harmful results. ";
            break;
        case MISC_AIR_ELEMENTAL_FAN:
            description += "A magical device for summoning air "
                "elementals. It is rather unreliable, and usually requires "
                "several attempts to function correctly. Using it carries "
                "an element of risk, which is reduced if one is skilled in "
                "the appropriate elemental magic. ";
            break;
        case MISC_LAMP_OF_FIRE:
            description += "A magical device for summoning fire "
                "elementals. It is rather unreliable, and usually "
                "requires several attempts to function correctly. Using "
                "it carries an element of risk, which is reduced if one "
                "is skilled in the appropriate elemental magic.";
            break;
        case MISC_STONE_OF_EARTH_ELEMENTALS:
            description += "A magical device for summoning earth "
                "elementals. It is rather unreliable, and usually "
                "requires several attempts to function correctly. "
                "Using it carries an element of risk, which is reduced "
                "if one is skilled in the appropriate elemental magic.";
            break;
        case MISC_LANTERN_OF_SHADOWS:
            description +=
                "An unholy device which calls on the powers of darkness "
                "to assist its user, with a small cost attached. ";
            break;
        case MISC_HORN_OF_GERYON:
            description +=
                "The horn belonging to Geryon, guardian of the Vestibule "
                "of Hell. Legends say that a mortal who desires access "
                "into one of the Hells must use it in order to gain entry. ";
            break;
        case MISC_BOX_OF_BEASTS:
            description +=
                "A magical box containing many wild beasts. One may "
                "allow them to escape by opening the box's lid. ";
            break;
        case MISC_DECK_OF_ESCAPE:
            description +=
                "A deck of magical cards, mainly dealing with various "
                "forms of escape. Incautious use may lead to being "
                "dumped from the frying pan into the fire! ";
            break;
        case MISC_DECK_OF_DESTRUCTION:
            description +=
                "A deck of magical cards, most of which hurl death "
                "and destruction at one's foes (or, if unlucky, at oneself.) ";
            break;
        case MISC_DECK_OF_DUNGEONS:
            description +=
                "A deck of magical cards which deal with shaping the "
                "dungeon. Unlike most other decks, the cards from this deck "
                "tend to be ignored by Nemelex, who prefers more amusing "
                "pursuits. ";
            break;
        case MISC_DECK_OF_SUMMONING:
            description +=
                "A deck of magical cards, depicting a range of weird and "
                "wondrous creatures. ";
            break;

        case MISC_DECK_OF_WONDERS:
            description +=
                "A deck of highly mysterious and magical cards, which can "
                "permanently alter the drawer's physical and mental "
                "condition, for better or worse. ";
            break;

        case MISC_DECK_OF_PUNISHMENT:
            description +=
                "A deck of magical cards which wreak havoc on the user. ";
            break;

        case MISC_DECK_OF_WAR:
            description +=
                "A deck of magical cards which are useful before and during "
                "battle. ";
            break;

        case MISC_DECK_OF_CHANGES:
            description +=
                "A deck of magical cards which induce changes in the user "
                "or his environment. ";
            break;
            
        case MISC_DECK_OF_DEFENSE:
            description +=
                "A deck of magical cards, most of which defend the user "
                "from harm in various ways. ";
            break;

        case MISC_CRYSTAL_BALL_OF_ENERGY:
            description +=
                "A magical device which can be used to restore one's "
                "reserves of magical energy, but the use of which carries "
                "the risk of draining all of those energies completely. "
                "This risk varies inversely with the proportion of their "
                "maximum energy which the user possesses; a user near his "
                "or her full potential will find this item most beneficial. ";
            break;
        case MISC_EMPTY_EBONY_CASKET:
            description += "A magical box after its power is spent. ";
            break;
        case MISC_CRYSTAL_BALL_OF_FIXATION:
            description +=
                "A dangerous item which hypnotises anyone so unwise as "
                "to gaze into it, leaving them helpless for a significant "
                "length of time. ";
            break;
        case MISC_DISC_OF_STORMS:
            description +=
                "This extremely powerful item can unleash a destructive "
                "storm of electricity. It is especially effective in the "
                "hands of one skilled in air elemental magic, but cannot "
                "be used by one who is not a conductor. ";
            break;

        case MISC_RUNE_OF_ZOT:
            description +=
                "A talisman which allows entry into Zot's domain. ";
            break;

        case NUM_MISCELLANY:
            description += "A buggy miscellaneous item.";
            break;
        }
    }
    else
    {
        switch (subtype)
        {
        case MISC_BOTTLED_EFREET:
            description += "A heavy bronze flask, warm to the touch. ";
            break;
        case MISC_CRYSTAL_BALL_OF_ENERGY:
        case MISC_CRYSTAL_BALL_OF_FIXATION:
        case MISC_CRYSTAL_BALL_OF_SEEING:
            description += "A sphere of clear crystal. ";
            break;
        case MISC_AIR_ELEMENTAL_FAN:
            description += "A fan. ";
            break;
        case MISC_LAMP_OF_FIRE:
            description += "A lamp. ";
            break;
        case MISC_STONE_OF_EARTH_ELEMENTALS:
            description += "A lump of rock. ";
            break;
        case MISC_LANTERN_OF_SHADOWS:
            description += "A strange lantern made out of ancient bones. ";
            break;
        case MISC_HORN_OF_GERYON:
            description += "A great silver horn, radiating unholy energies. ";
            break;
        case MISC_BOX_OF_BEASTS:
        case MISC_EMPTY_EBONY_CASKET:
            description += "A small black box. I wonder what's inside? ";
            break;

        case MISC_DECK_OF_ESCAPE:
        case MISC_DECK_OF_DESTRUCTION:
        case MISC_DECK_OF_DUNGEONS:
        case MISC_DECK_OF_SUMMONING:
        case MISC_DECK_OF_WONDERS:
        case MISC_DECK_OF_PUNISHMENT:
        case MISC_DECK_OF_WAR:
        case MISC_DECK_OF_CHANGES:
        case MISC_DECK_OF_DEFENSE:
            description += "A deck of cards. ";
            break;
        case MISC_RUNE_OF_ZOT:
            description += "A talisman of some sort. ";
            break;
        case MISC_DISC_OF_STORMS:
            description += "A grey disc. ";
            break;
        case NUM_MISCELLANY:
            description += "A buggy miscellaneous item. ";
            break;
        }
    }

    description += "$";

    if ( is_deck(item) && item.plus2 != 0 )
    {
        description += "$Next card(s): ";
        description += card_name(static_cast<card_type>(item.plus2 - 1));
        long spec = item.special;
        while ( spec )
        {
            description += ", ";
            description += card_name(static_cast<card_type>((spec & 0xFF)-1));
            spec >>= 8;
        }
        description += "$";
    }

    return (description);
}

// ========================================================================
//      Public Functions
// ========================================================================

bool is_dumpable_artefact( const item_def &item, bool verbose)
{
    bool ret = false;

    if (is_artefact( item ) )
    {
        ret = item_ident( item, ISFLAG_KNOW_PROPERTIES );
    }
    else if (item.base_type == OBJ_ARMOUR
        && (verbose && item_type_known(item)))
    {
        const int spec_ench = get_armour_ego_type( item );
        ret = (spec_ench >= SPARM_RUNNING && spec_ench <= SPARM_PRESERVATION);
    }
    else if (item.base_type == OBJ_JEWELLERY 
        && (verbose && item_type_known(item)))
    {
        ret = true;
    }

    return (ret);
}


//---------------------------------------------------------------
//
// get_item_description
//
// Note that the string will include dollar signs which should
// be interpreted as carriage returns.
//
//---------------------------------------------------------------
std::string get_item_description( const item_def &item, bool verbose,
                                  bool dump )
{
    std::ostringstream description;

    if (!dump)
        description << item.name(DESC_INVENTORY_EQUIP);

    description << "$$";

#if DEBUG_DIAGNOSTICS
    if (!dump)
    {
        description << std::setfill('0');
        description << "base: " << static_cast<int>(item.base_type)
                    << " sub: " << static_cast<int>(item.sub_type)
                    << " plus: " << item.plus << " plus2: " << item.plus2
                    << " special: " << item.special
                    << "$"
                    << "quant: " << item.quantity
                    << " colour: " << static_cast<int>(item.colour)
                    << " flags: " << std::hex << std::setw(8) << item.flags
                    << std::dec << "$"
                    << "x: " << item.x << " y: " << item.y
                    << " link: " << item.link
                    << " ident_type: "
                    << static_cast<int>(get_ident_type(item.base_type,
                                                       item.sub_type))
                    << "$$";
    }
#endif

    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        description << describe_weapon( item, verbose );
        break;
    case OBJ_MISSILES:
        description << describe_ammo( item );
        break;
    case OBJ_ARMOUR:
        description << describe_armour( item, verbose );
        break;
    case OBJ_WANDS:
        description << describe_stick( item );
        break;
    case OBJ_FOOD:
        description << describe_food( item );
        break;
    case OBJ_SCROLLS:
        description << describe_scroll( item );
        break;
    case OBJ_JEWELLERY:
        description << describe_jewellery( item, verbose );
        break;
    case OBJ_POTIONS:
        description << describe_potion( item );
        break;
    case OBJ_STAVES:
        description << describe_staff( item );
        break;

    case OBJ_BOOKS:
        switch (item.sub_type)
        {
        case BOOK_DESTRUCTION:
            description << "An extremely powerful but unpredictable book "
                "of magic. ";
            break;

        case BOOK_MANUAL:
            description << "A valuable book of magic which allows one to "
                "practise a certain skill greatly. As it is used, it "
                "gradually disintegrates and will eventually fall apart. ";
            break;

        default:
            description << "A book of magic spells. Beware, for some of the "
                "more powerful grimoires are not to be toyed with. ";
            break;
        }
        break;

    case OBJ_ORBS:
        description << "Once you have escaped to the surface with "
            "this invaluable artefact, your quest is complete. ";
        break;

    case OBJ_MISCELLANY:
        description << describe_misc_item( item );
        break;

    case OBJ_CORPSES:
        if ( item.sub_type == CORPSE_BODY)
            description << "A corpse. ";
        else
            description << "A decaying skeleton. ";
        break;

    case OBJ_GOLD:
        description << "A pile of glittering gold coins. ";
        break;

    default:
        DEBUGSTR("Bad item class");
        description << "This item should not exist. Mayday! Mayday! ";
    }

    if (verbose)
    {
        const int mass = item_mass( item );
        description << "$It weighs around " << (mass / 10)
                    << "." << (mass % 10)
                    << " aum. "; // arbitrary unit of mass

        if ( is_dumpable_artefact(item, false) )
        {
            if (item.base_type == OBJ_ARMOUR || item.base_type == OBJ_WEAPONS)
                description << "$$This ancient artefact cannot be changed "
                    "by magic or mundane means.";
            else
                description << "$$It is an ancient artefact.";
        }
    }

    return description.str();
}                               // end get_item_description()

static std::string get_feature_description_wide(int feat)
{
    return std::string();
}

void describe_feature_wide(int x, int y)
{
    std::string desc = feature_description(x, y);
    desc += "$$";

    // Get rid of trailing .$$ before lookup
    desc +=
        getLongDescription(grd[x][y] == DNGN_ENTER_SHOP? "A shop"
                           : desc.substr(0, desc.length() - 3));

    // For things which require logic
    desc += get_feature_description_wide(grd[x][y]);

    clrscr();
    print_description(desc);
    
    if (Options.tutorial_left)
    {
        gotoxy(1, wherey() + 2);
        tutorial_describe_feature(grd[x][y]);
    }
        
    if ( getch() == 0 )
        getch();
}

static void show_item_description(const item_def &item)
{
    clrscr();

    const std::string description = get_item_description( item, 1 );
    print_description(description);

    if (item.has_spells())
    {
        formatted_string fs;
        item_def dup = item;
        spellbook_contents( dup, 
                item.base_type == OBJ_BOOKS? 
                    RBOOK_READ_SPELL
                  : RBOOK_USE_STAFF, 
                &fs );
        fs.display(2, -2);
    }
}

static bool describe_spells(const item_def &item)
{
    int c = getch();
    if (c < 'a' || c > 'h')     //jmf: was 'g', but 8=h
    {
        mesclr( true );
        return (false);
    }

    const int spell_index = letter_to_index(c);

    spell_type nthing =
        which_spell_in_book(item.book_number(), spell_index);
    if (nthing == SPELL_NO_SPELL)
        return (false);

    describe_spell( nthing );
    return (true);
}

//---------------------------------------------------------------
//
// describe_item
//
// Describes all items in the game.
//
//---------------------------------------------------------------
void describe_item( item_def &item, bool allow_inscribe )
{
    for (;;)
    {
        show_item_description(item);
        if (item.has_spells())
        {
            gotoxy(1, wherey());
            textcolor(LIGHTGREY);
            cprintf("Select a spell to read its description.");
            if (!describe_spells(item))
                break;
            continue;
        }
        break;
    }
    
    if (Options.tutorial_left)
    {
        tutorial_describe_item(item);
        getch();
    }
    // Don't ask during tutorial, or if there aren't enough rows left
    else if (allow_inscribe && wherey() <= get_number_of_lines() - 3)
    {
        gotoxy(1, wherey() + 2);
        formatted_string::parse_string("<cyan>Do you wish to inscribe this item? ").display();

        if (toupper(getch()) == 'Y')
        {
            char buf[79];
            cprintf("\nInscribe with what? ");
            if (!cancelable_get_line(buf, sizeof buf))
            {
                item.inscription = std::string(buf);
            }
        }
    }
    else if (getch() == 0)
        getch();

}


//---------------------------------------------------------------
//
// describe_spell
//
// Describes (most) every spell in the game.
//
//---------------------------------------------------------------
void describe_spell(spell_type spelled)
{
    std::string description;

    description.reserve(500);

    description += spell_title( spelled );
    description += "$$";
    const std::string long_descrip = getLongDescription(spell_title(spelled));
    if ( !long_descrip.empty() )
        description += long_descrip;
    else
    {
        description += "This spell has no description. "
            "Casting it may therefore be unwise. "
#if DEBUG
            "Instead, go fix it. ";
#else
        "Please file a bug report.";
#endif // DEBUG
    }

    clrscr();
    print_description(description);

    if (getch() == 0)
        getch();
}                               // end describe_spell()

static std::string describe_draconian_role(const monsters *mon)
{
    switch (mon->type)
    {
    case MONS_DRACONIAN_SHIFTER:
        return "It darts around disconcertingly without taking a step.";
    case MONS_DRACONIAN_SCORCHER:
        return "Its scales are sooty black from years of magical pyrotechnics.";
    case MONS_DRACONIAN_ZEALOT:
        return "In its gaze you see all the malefic power of its "
               "terrible god.";
    case MONS_DRACONIAN_ANNIHILATOR:
        return "Crackling balls of pure energy hum and spark up and down its "
               "scaled fists and arms.";
    case MONS_DRACONIAN_CALLER:
        return "It looks especially reptilian, and eager for company.";
    case MONS_DRACONIAN_MONK:
        return "It looks unnaturally strong and dangerous with its fists.";
    case MONS_DRACONIAN_KNIGHT:
        return "It wields a deadly weapon with menacing efficiency.";
    }
    return ("");
}

static std::string describe_draconian_colour(int species)
{
    switch (species)
    {
    case MONS_BLACK_DRACONIAN:
        return "Sparks crackle and flare out of its mouth and nostrils.";
    case MONS_MOTTLED_DRACONIAN:
        return "Liquid flames drip from its mouth.";
    case MONS_YELLOW_DRACONIAN:
        return "Acid fumes swirl around it.";
    case MONS_GREEN_DRACONIAN:
        return "Venom steams and drips from its jaws.";
    case MONS_PURPLE_DRACONIAN:
        return "Its outline shimmers with wild energies.";
    case MONS_RED_DRACONIAN:
        return "Smoke pours from its nostrils.";
    case MONS_WHITE_DRACONIAN:
        return "Frost pours from its nostrils.";
    case MONS_PALE_DRACONIAN:
        return "It is cloaked in a pall of superheated steam.";
    }
    return ("");
}

static std::string describe_draconian(const monsters *mon)
{
    std::string description;      
    const int subsp = draco_subspecies( mon );

    if (subsp == MONS_DRACONIAN)
        description += "A ";
    else
        description += "A muscular ";

    switch (subsp)
    {
    case MONS_DRACONIAN:            description += "brown-";   break;
    case MONS_BLACK_DRACONIAN:      description += "black-";   break;
    case MONS_MOTTLED_DRACONIAN:    description += "mottled-"; break;
    case MONS_YELLOW_DRACONIAN:     description += "yellow-";  break;
    case MONS_GREEN_DRACONIAN:      description += "green-";   break;
    case MONS_PURPLE_DRACONIAN:     description += "purple-";  break;
    case MONS_RED_DRACONIAN:        description += "red-";     break;
    case MONS_WHITE_DRACONIAN:      description += "white-";   break;
    case MONS_PALE_DRACONIAN:       description += "pale-";    break;
    default:
        break;
    }

    description += "scaled humanoid with wings.";
    
    if (subsp != MONS_DRACONIAN)
        if (describe_draconian_colour(subsp) != "")
            description += "  " + describe_draconian_colour(subsp);

    if (subsp != mon->type)
        if (describe_draconian_role(mon) != "")
            description += "  " + describe_draconian_role(mon);

    return (description);
}

//---------------------------------------------------------------
//
// describe_monsters
//
// Contains sketchy descriptions of every monster in the game.
//
//---------------------------------------------------------------
void describe_monsters(monsters& mons)
{
    std::ostringstream description;

    description << mons.name(DESC_CAP_A) << "$$";
    
    // Note: Nearly all of the "long" descriptions have moved to 
    // mon-data.h, in an effort to give them some locality with the
    // rest of the monster definition data, and to get them out of
    // the control logic.  The only sorts of descriptions that should
    // go here are those that require active decisions about what
    // sort of message to print (eg "It's beautiful" or "It's ugly" 
    // depending on player class).  And just between you and me: that's
    // sort of a dumb idea anyway.  So don't add any more of those.
    //
    // In my fantasy world, the long descriptions (and monster data)
    // wouldn't live in a header file, but in a simple text file
    // that's easier to edit and easy to read.  Even XML would be better
    // than what we have today.
    //
    // -peterb 4/14/07
    description << getLongDescription(mons.name(DESC_PLAIN));

    std::string symbol = "";
    symbol += get_monster_data(mons.type)->showchar;
    if (isupper(symbol[0]))
        symbol = "cap-" + symbol;

    std::string symbol_prefix = "__";
    symbol_prefix += symbol;
    symbol_prefix += "_prefix";
    description << getLongDescription(symbol_prefix);

    // Now that the player has examined it, he knows it's a mimic.
    if (mons_is_mimic(mons.type))
        mons.flags |= MF_KNOWN_MIMIC;

    switch (mons.type)
    {
    case MONS_ZOMBIE_SMALL: case MONS_ZOMBIE_LARGE:
        description << getLongDescription("zombie");
        break;
        
    case MONS_SKELETON_SMALL: case MONS_SKELETON_LARGE:
        description << getLongDescription("skeleton");
        break;

    case MONS_SIMULACRUM_SMALL: case MONS_SIMULACRUM_LARGE:
        description << getLongDescription("simulacrum");
        break;

    case MONS_SPECTRAL_THING:
        description << getLongDescription("spectral thing");
        break;

    case MONS_ROTTING_DEVIL:
        if (you.species == SP_GHOUL)
            description << "It smells great!";
        else if (player_can_smell())
            description << "It stinks.";
        break;

    case MONS_SWAMP_DRAKE:
        if (player_can_smell())
            description << "It smells horrible.";
        break;

    case MONS_NAGA:
    case MONS_NAGA_MAGE:
    case MONS_NAGA_WARRIOR:
    case MONS_GUARDIAN_NAGA:
    case MONS_GREATER_NAGA:
        if (you.species == SP_NAGA)
            description << "It is particularly attractive.";
        else
            description << "It is strange and repulsive.";
        break;

    case MONS_VAMPIRE:
    case MONS_VAMPIRE_KNIGHT:
    case MONS_VAMPIRE_MAGE:
        if (you.is_undead == US_ALIVE)
            description << " It wants to drink your blood! ";
        break;

    case MONS_REAPER:
        if (you.is_undead == US_ALIVE)
            description << "It has come for your soul!";
        break;

    case MONS_ELF:
        // These are only possible from polymorphing or shapeshifting.
        description << "This one is remarkably plain looking.";
        break;

    case MONS_DRACONIAN:
    case MONS_RED_DRACONIAN:
    case MONS_WHITE_DRACONIAN:
    case MONS_GREEN_DRACONIAN:
    case MONS_PALE_DRACONIAN:
    case MONS_MOTTLED_DRACONIAN:
    case MONS_BLACK_DRACONIAN:
    case MONS_YELLOW_DRACONIAN:
    case MONS_PURPLE_DRACONIAN:
    case MONS_DRACONIAN_SHIFTER:
    case MONS_DRACONIAN_SCORCHER:
    case MONS_DRACONIAN_ZEALOT:
    case MONS_DRACONIAN_ANNIHILATOR:
    case MONS_DRACONIAN_CALLER:
    case MONS_DRACONIAN_MONK:
    case MONS_DRACONIAN_KNIGHT:
    {
        description << describe_draconian( &mons );
        break;        
    }
    case MONS_PLAYER_GHOST:
        description << "The apparition of " << ghost_description(mons) << ".$";
        break;

    case MONS_PANDEMONIUM_DEMON:
        description << describe_demon(mons);
        break;

    case MONS_URUG:
        if (player_can_smell())
            description << "He smells terrible.";
        break;

    case MONS_PROGRAM_BUG:
        description << "If this monster is a \"program bug\", then it's "
            "recommended that you save your game and reload.  Please report "
            "monsters who masquerade as program bugs or run around the "
            "dungeon without a proper description to the authorities.";
        break;
    default:
        break;
    }

    description << "\n";
    std::string symbol_suffix = "__";
    symbol_suffix += symbol;
    symbol_suffix += "_suffix";
    description << getLongDescription(symbol_suffix);

#if DEBUG_DIAGNOSTICS

    if (mons_class_flag( mons.type, M_SPELLCASTER ))
    {
        const monster_spells &hspell_pass = mons.spells;
        bool found_spell = false;

        for (int i = 0; i < NUM_MONSTER_SPELL_SLOTS; i++)
        {
            if (hspell_pass[i] != SPELL_NO_SPELL)
            {
                if (!found_spell)
                {
                    description << "$$Monster Spells:$";
                    found_spell = true;
                }

                description << "    " << i << ": "
                            << spell_title(hspell_pass[i])
                            << " (" << static_cast<int>(hspell_pass[i])
                            << ")$";
            }
        }
    }

    bool has_item = false;
    for (int i = 0; i < NUM_MONSTER_SLOTS; i++)
    {
        if (mons.inv[i] != NON_ITEM)
        {
            if (!has_item)
            {
                description << "$Monster Inventory:$";
                has_item = true;
            }
            description << "    " << i << ") "
                        << mitm[mons.inv[i]].name(DESC_NOCAP_A, false, true);
        }
    }
#endif

    clrscr();
    print_description(description.str());

    if (Options.tutorial_left)
    {
        gotoxy(1, wherey() + 2);
        tutorial_describe_monster(static_cast<const monsters*>(&mons));
    }

    if (getch() == 0)
        getch();
}                               // end describe_monsters

//---------------------------------------------------------------
//
// ghost_description
//
// Describes the current ghost's previous owner. The caller must
// prepend "The apparition of" or whatever and append any trailing
// punctuation that's wanted.
//
//---------------------------------------------------------------
std::string ghost_description(const monsters &mons, bool concise)
{
    ASSERT(mons.ghost.get());
    std::ostringstream gstr;

    const ghost_demon &ghost = *(mons.ghost);

    // We're fudging stats so that unarmed combat gets based off
    // of the ghost's species, not the player's stats... exact 
    // stats aren't required anyways, all that matters is whether
    // dex >= str. -- bwr
    const int dex = 10;
    int str;
    switch (ghost.values[GVAL_SPECIES])
    {
    case SP_MOUNTAIN_DWARF:
    case SP_TROLL:
    case SP_OGRE:
    case SP_OGRE_MAGE:
    case SP_MINOTAUR:
    case SP_HILL_ORC:
    case SP_CENTAUR:
    case SP_NAGA:
    case SP_MUMMY:
    case SP_GHOUL:
        str = 15;
        break;

    case SP_HUMAN:
    case SP_DEMIGOD:
    case SP_DEMONSPAWN:
        str = 10;
        break;

    default:
        str = 5;
        break;
    }

    gstr << ghost.name << " the "
         << skill_title( ghost.values[GVAL_BEST_SKILL], 
                         ghost.values[GVAL_SKILL_LEVEL],
                         ghost.values[GVAL_SPECIES], 
                         str, dex, GOD_NO_GOD )
         << ", a"
         << ((ghost.values[GVAL_EXP_LEVEL] <  4) ? " weakling" :
             (ghost.values[GVAL_EXP_LEVEL] <  7) ? "n average" :
             (ghost.values[GVAL_EXP_LEVEL] < 11) ? "n experienced" :
             (ghost.values[GVAL_EXP_LEVEL] < 16) ? " powerful" :
             (ghost.values[GVAL_EXP_LEVEL] < 22) ? " mighty" :
             (ghost.values[GVAL_EXP_LEVEL] < 26) ? " great" :
             (ghost.values[GVAL_EXP_LEVEL] < 27) ? "n awesomely powerful"
                                                 : " legendary")
         << " ";
    if ( concise )
        gstr << get_species_abbrev(ghost.values[GVAL_SPECIES])
             << get_class_abbrev(ghost.values[GVAL_CLASS]);
    else
        gstr << species_name(ghost.values[GVAL_SPECIES], 
                             ghost.values[GVAL_EXP_LEVEL])
             << " "
             << get_class_name(ghost.values[GVAL_CLASS]);

    return gstr.str();
}

extern ability_type god_abilities[MAX_NUM_GODS][MAX_GOD_ABILITIES];

static bool print_god_abil_desc( int god, int numpower )
{
    const char* pmsg = god_gain_power_messages[god][numpower];

    // if no message then no power
    if ( !pmsg[0] )
        return false;
    
    std::ostringstream buf;

    if ( isupper(pmsg[0]) )
        buf << pmsg;            // complete sentence given
    else
        buf << "You can " << pmsg << ".";

    // this might be ABIL_NON_ABILITY for passive abilities
    const ability_type abil = god_abilities[god][numpower];

    if ( abil != ABIL_NON_ABILITY )
    {
        const int spacesleft = 79 - buf.str().length();
        const std::string cost = "(" + make_cost_description(abil) + ")";
        buf << std::setw(spacesleft) << cost;
    }
    
    cprintf( "%s\n", buf.str().c_str() );
    return true;
}

static std::string describe_favour_generic(god_type which_god)
{
    std::string godname = god_name(which_god);
    return (you.piety > 130) ? "A prized avatar of " + godname + ".":
        (you.piety > 100) ? "A shining star in the eyes of " + godname + "." :
        (you.piety >  70) ? "A rising star in the eyes of " + godname + "." :
        (you.piety >  40) ? godname + " is most pleased with you." :
        (you.piety >  20) ? godname + " has noted your presence." :
        (you.piety >   5) ? godname + " is noncommittal."
        : "You are beneath notice.";
}

//---------------------------------------------------------------
//
// describe_god
//
// Describes the player's standing with his deity. 
//
//---------------------------------------------------------------

std::string describe_favour(god_type which_god)
{
    if (player_under_penance())
    {
        const int penance = you.penance[which_god];
        return (penance >= 50) ? "Godly wrath is upon you!" :
            (penance >= 20) ? "You've transgressed heavily! Be penitent!" :
            (penance >= 5 ) ? "You are under penance." 
                            : "You should show more discipline.";
    }

    return (which_god == GOD_XOM)?
            describe_xom_favour() : describe_favour_generic(which_god);
}

static std::string religion_help( god_type god )
{
    std::string result = "";
    
    switch(god)
    {
    case GOD_ZIN:
    case GOD_SHINING_ONE:
        if (!player_under_penance() && you.piety > 160
            && !you.num_gifts[god])
        {
            result += "You can pray at an altar to ask "
                + std::string(god_name(god)) + " to bless a ";
            result += (god == GOD_ZIN ? "mace" : "long sword");
            result += ".";
        }
        break;

    case GOD_LUGONU:
        if (!player_under_penance() && you.piety > 160
            && !you.num_gifts[god])
        {
            result += "You can pray at an altar to ask "
                + std::string(god_name(god)) + " to bless your weapon." EOL;
        } // fall through
           
    case GOD_OKAWARU:
    case GOD_MAKHLEB:
    case GOD_BEOGH:
    case GOD_TROG:
        result = "You can sacrifice corpses by dissecting "
                 "them during prayer.";
        break;

    case GOD_NEMELEX_XOBEH:
        result += "You can pray to sacrifice all items on your square. "
             "Inscribe items with !p, !* or =p to avoid sacrificing "
             "them accidentally.";
        break;

    case GOD_VEHUMET:
        if (you.piety >= 50)
            result += "Vehumet assists you in casting Conjurations"
                      " and Summonings during prayer.";
        break;

    default:
        break;
    }
    return result;
}

void describe_god( god_type which_god, bool give_title )
{
    int         colour;      // mv: colour used for some messages

    clrscr();

    if (give_title)
    {
        textcolor( WHITE );
        cprintf( "                                  Religion" EOL );
        textcolor( LIGHTGREY );
    }

    if (which_god == GOD_NO_GOD) //mv:no god -> say it and go away
    {
        cprintf( EOL "You are not religious." );
        get_ch();
        return;
    }

    colour = god_colour(which_god);

    //mv: print god's name and title - if you can think up better titles
    //I have nothing against
    textcolor(colour);
    cprintf( "%s", god_name(which_god,true)); //print long god's name
    cprintf (EOL EOL);

    //mv: print god's description
    textcolor(LIGHTGRAY);

    const char* god_descriptions[NUM_GODS] = {
        // GOD_NO_GOD
        "",
        // Zin
        "Zin is an ancient and revered God, dedicated to the establishment of "
        "order and the destruction of the forces of chaos and night. Valued "
        "worshippers can gain sustenance in times of need, blessings on their "
        "weapons, and a variety of powers useful in the fight against evil, "
        "but must abstain from the use of necromancy and other forms of "
        "unholy magic. Zin appreciates long-standing faith as well as "
        "sacrifices of valued objects.",

        // TSO
        "The Shining One is a powerful crusading deity, allied with Zin in "
        "the fight against evil. Followers may be granted blessings on their "
        "weapons and the ability to summarily dispense the wrath of heaven, "
        "but must never use any form of evil magic and should fight "
        "honourably. The Shining One appreciates long-standing persistence in "
        "the endless crusade, as well as the dedicated destruction of unholy "
        "creatures.",

        // Kikubaaqudgha
        "Kikubaaqudgha is a terrible Demon-God, served by those who seek "
        "knowledge of the powers of death. Followers gain special powers over "
        "the undead, and especially favoured servants can call on mighty "
        "demons to slay their foes. Kikubaaqudgha requires the deaths of "
        "living creatures as often as possible, but is not interested in the "
        "offering of corpses except at an appropriate altar.",

        // Yredelemnul
        "Yredelemnul is worshipped by those who seek powers over death and "
        "the undead without having to learn to use necromancy. Followers can "
        "raise legions of servile undead and gain a number of other useful "
        "(if unpleasant) powers. Yredelemnul appreciates killing, but prefers "
        "corpses to be put to use rather than sacrificed.",

        // Xom
        "Xom is a wild and unpredictable God of chaos, who seeks not "
        "worshippers but playthings to toy with. Many choose to follow Xom "
        "in the hope of receiving fabulous rewards and mighty powers, but Xom "
        "is nothing if not capricious. ",

        // Vehumet
        "Vehumet is a God of the destructive powers of magic. Followers gain "
        "various useful powers to enhance their command of the hermetic arts, "
        "and the most favoured stand to gain access to some of the fearsome "
        "spells in Vehumet's library. One's devotion to Vehumet can be proved "
        "by the causing of as much carnage and destruction as possible.",

        // Okawaru
        "Okawaru is a dangerous and powerful God of battle. Followers can "
        "gain a number of powers useful in combat as well as various rewards, "
        "but must constantly prove themselves through battle and the "
        "sacrifice of corpses and valuable items. Okawaru despises those who "
        "harm their allies.",

        // Makhleb
        "Makhleb the Destroyer is a fearsome God of chaos and violent death. "
        "Followers, who must constantly appease Makhleb with blood, stand to "
        "gain various powers of death and destruction. The Destroyer "
        "appreciates sacrifices of corpses and valuable items.",

        // Sif Muna
        "Sif Muna is a contemplative but powerful deity, served by those who "
        "seek magical knowledge. Sif Muna appreciates sacrifices of valuable "
        "items, and the casting of spells as often as possible.",

        // Trog
        "Trog is an ancient God of anger and violence. Followers are expected "
        "to kill in Trog's name and sacrifice the dead, and in return gain "
        "power in battle and occasional rewards. Trog hates wizards, and loves "
        "to see their magical books burn. Followers are forbidden the use of "
        "spell magic. ",

        // Nemelex Xobeh
        "Nemelex is a strange and unpredictable trickster God, whose powers "
        "can be invoked through the magical packs of cards which Nemelex "
        "paints in the ichor of demons. Followers receive occasional gifts, "
        "and should use these as much as possible. Offerings of any "
        "type of item are also appreciated. The Trickster may provide certain "
        "ways to improve one's chances at cards, but prefers those who trust "
        "to luck.",

        // Elyvilon
        "Elyvilon the Healer is worshipped by the healers (among others), who "
        "gain their healing powers by long worship and devotion. Although "
        "Elyvilon prefers a creed of pacifism, those who crusade against evil "
        "are not excluded. Elyvilon appreciates the destruction of weapons.",

        // Lugonu
        "Lugonu the Unformed revels in the chaos of the Abyss. Followers are "
        "sent out to cause bloodshed and disorder in the world, and must do "
        "so unflaggingly to earn Lugonu's favour. Lugonu expects followers "
        "to spread chaos and corruption in the overworld.",

        // Beogh
        "Beogh is a deity worshipped by the cave orcs native to parts of the "
        "dungeon. Only orcs may devote their service to Beogh, and must prove "
        "their devotion by bloodshed and sacrifice. Devout followers of Beogh "
        "can smite their foes, and especially fervent devotees of Beogh may "
        "even gain followers of their own, for the orcs still look for their "
        "Messiah."
    };

    const int numcols = get_number_of_cols();
    cprintf("%s", get_linebreak_string(god_descriptions[which_god],
                                       numcols).c_str());

    // title only shown for our own god
    if (you.religion == which_god)
    {
        //mv: print title based on piety
        cprintf( EOL EOL "Title - " );
        textcolor(colour);

        // mv: if your piety is high enough you get title
        // based on your god
        if (you.piety > 160) 
        { 
            cprintf(
                "%s", (which_god == GOD_SHINING_ONE) ? "Champion of Law" :
                (which_god == GOD_ZIN) ? "Divine Warrior" :
                (which_god == GOD_ELYVILON) ? "Champion of Light" :
                (which_god == GOD_OKAWARU) ? "Master of a Thousand Battles" :
                (which_god == GOD_YREDELEMNUL) ? "Master of Eternal Death" :
                (which_god == GOD_KIKUBAAQUDGHA) ? "Lord of Darkness" :
                (which_god == GOD_MAKHLEB) ? "Champion of Chaos" :
                (which_god == GOD_VEHUMET) ? "Lord of Destruction" :
                (which_god == GOD_TROG) ? "Great Slayer" :
                (which_god == GOD_NEMELEX_XOBEH) ? "Great Trickster" :
                (which_god == GOD_SIF_MUNA) ? "Master of the Arcane" :
                (which_god == GOD_LUGONU) ? "Agent of Entropy" :
                (which_god == GOD_BEOGH) ? "Messiah" :
                (which_god == GOD_XOM) ? "Teddy Bear" : 
                "Bogy the Lord of the Bugs");
        }
        else
        {
            //mv: most titles are still universal - if any one wants to
            //he might write specific titles for all gods or rewrite current
            //ones (I know they are not perfect)
            //btw. titles are divided according to piety levels on which you get
            //new abilities.In the main it means - new ability = new title
            switch (which_god)
            {
            case GOD_ZIN:
            case GOD_SHINING_ONE:
            case GOD_KIKUBAAQUDGHA:
            case GOD_YREDELEMNUL:
            case GOD_VEHUMET:
            case GOD_OKAWARU:
            case GOD_MAKHLEB:
            case GOD_TROG:
            case GOD_NEMELEX_XOBEH:
            case GOD_ELYVILON:
            case GOD_LUGONU:
                cprintf ( (you.piety >= 120) ? "High Priest" :
                          (you.piety >= 100) ? "Elder" :
                          (you.piety >=  75) ? "Priest" :
                          (you.piety >=  50) ? "Deacon" :
                          (you.piety >=  30) ? "Novice" :
                          (you.piety >    5) ? "Believer"
                                             : "Sinner" );
                break;
            case GOD_SIF_MUNA:
                cprintf ( (you.piety >= 120) ? "Oracle" :
                          (you.piety >= 100) ? "Scholar" :
                          (you.piety >=  75) ? "Adept" :
                          (you.piety >=  50) ? "Disciple" :
                          (you.piety >=  30) ? "Apprentice" :
                          (you.piety >    5) ? "Believer"
                                             : "Sinner" );
                break;
            case GOD_BEOGH:
                cprintf ( (you.piety >= 120) ? "Saint" :
                          (you.piety >= 100) ? "High Priest" :
                          (you.piety >=  75) ? "Missionary" :
                          (you.piety >=  50) ? "Priest" :
                          (you.piety >=  30) ? "Disciple" :
                          (you.piety >    5) ? "Believer"
                                             : "Sinner" );
                 break;
            case GOD_XOM:
                cprintf("Toy");
            break;

            default: 
                cprintf ("Bug");
            }
        }
    }
    // end of print title

    // mv: now let's print favor as Brent suggested
    // I know these messages aren't perfect so if you can
    // think up something better, do it

    textcolor(LIGHTGRAY);
    cprintf(EOL EOL "Favour - ");
    textcolor(colour);

    //mv: player is praying at altar without appropriate religion
    //it means player isn't checking his own religion and so we only
    //display favour and will go out
    if (you.religion != which_god)
    {
        textcolor (colour);
        cprintf( (you.penance[which_god] >= 50) ? "%s's wrath is upon you!" :
                 (you.penance[which_god] >= 20) ? "%s is annoyed with you." :
                 (you.penance[which_god] >=  5) ? "%s well remembers your sins." :
                 (you.penance[which_god] >   0) ? "%s is ready to forgive your sins." :
                 (you.worshipped[which_god])    ? "%s is ambivalent towards you." 
                                                : "%s is neutral towards you.",
                 god_name(which_god) );
    } 
    else
    {
        cprintf(describe_favour(which_god).c_str());

        //mv: following code shows abilities given from god (if any)
        textcolor(LIGHTGRAY);
        cprintf(EOL EOL "Granted powers:                                                          (Cost)" EOL);
        textcolor(colour);

        // mv: these gods protects you during your prayer (not mentioning XOM)
        // chance for doing so is (random2(you.piety) >= 30)
        // Note that it's not depending on penance.
        // Btw. I'm not sure how to explain such divine protection
        // because god isn't really protecting player - he only sometimes
        // saves his life (probably it shouldn't be displayed at all).
        // What about this ?
        bool have_any = false;
        if ((which_god == GOD_ZIN
                || which_god == GOD_SHINING_ONE
                || which_god == GOD_ELYVILON
                || which_god == GOD_YREDELEMNUL)
            && you.piety >= 30)
        {
            have_any = true;
            cprintf( "%s %s watches over you during prayer." EOL,
                     god_name(which_god),
                     (you.piety >= 150) ? "carefully":   // > 4/5
                     (you.piety >=  90) ? "often" :      // > 2/3
                                          "sometimes");  // less than 2:3

            if (which_god == GOD_ZIN)
            {
                cprintf("Praying to %s will provide sustenance if starving."
                        EOL, god_name(which_god));
            }
        }

        if (which_god == GOD_TROG)
        {
            have_any = true;
            cprintf("You can call upon %s to burn books in your surroundings."
                    EOL, god_name(which_god));
        }
        else if (which_god == GOD_ELYVILON)
        {
            have_any = true;
            cprintf("You can call upon %s to destroy weapons "
                    "lying on the ground." EOL, god_name(which_god));
        }

        // mv: No abilities (except divine protection) under penance
        if (!player_under_penance())
        {
            for ( int i = 0; i < MAX_GOD_ABILITIES; ++i )
                if ( you.piety >= piety_breakpoint(i) )
                    if (print_god_abil_desc(which_god, i))
                        have_any = true;
        }
        if ( !have_any )
            cprintf( "None." EOL );
    }

    // only give this additional information for worshippers
    if ( which_god == you.religion )
    {
        if (you.religion == GOD_ZIN)
            gotoxy(1, get_number_of_lines() - 1);
        else
            gotoxy(1, get_number_of_lines() - 2);
        
        textcolor(LIGHTGRAY);
        cprintf(get_linebreak_string(religion_help(which_god),
                                     numcols).c_str());
    }

    get_ch();
}
