#include "protobotinsults.h"

#include "random.h"
#include "timer.h"

// neo-dimension.wikidot.com/somi:insult-swordfighting
ProtoBotInsults::ProtoBotInsults()
    : _insults{
         "You fight like a dairy farmer.",
         "Soon you'll be wearing my sword like a shish-kabob!",
         "You're no match for my brains, you poor fool.",
         "This is the end for you, you gutter-crawling cur!",
         "There are no words for how disgusting you are.",
         "I'm not going to take your insolence sitting down!",
         "People fall at my feet when they see me coming.",
         "I've heard you are a contemptible sneak.",
         "My handkerchief will wipe up your blood!",
         "If your brother's like you, better to marry a pig.",
         "Every word you say to me is stupid.",
         "My last fight ended with my hands covered in blood.",
         "You are a pain in the backside, sir!",
         "My sword is famous all over the Caribbean!",
         "There are no clever moves that can help you now.",
         "I will milk every drop of blood from your body!",
         "I hope you have a boat ready for a quick escape.",
         "My name is feared in every dirty corner of this island!"
      }
{
   shootAgain();
}

void ProtoBotInsults::shootAgain()
{
   Timer::singleShot(20000 + Random::bounded(60000), [this]() { insult(); });
}

void ProtoBotInsults::insult()
{
   if (!_insults.empty())
   {
      sendMessageSignal(_insults[Random::bounded(static_cast<int32_t>(_insults.size()) - 1)]);
      shootAgain();
   }
}
