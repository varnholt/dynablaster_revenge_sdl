#include "protobotinsults.h"

#include "random.h"

#include "timer.h"

ProtoBotInsults::ProtoBotInsults()
{
   _insults.push_back("You fight like a dairy farmer.");
   _insults.push_back("Soon you'll be wearing my sword like a shish-kabob!");
   _insults.push_back("You're no match for my brains, you poor fool.");
   _insults.push_back("This is the end for you, you gutter-crawling cur!");
   _insults.push_back("There are no words for how disgusting you are.");
   _insults.push_back("I'm not going to take your insolence sitting down!");
   _insults.push_back("People fall at my feet when they see me coming.");
   _insults.push_back("I've heard you are a contemptible sneak.");
   _insults.push_back("My handkerchief will wipe up your blood!");
   _insults.push_back("If your brother's like you, better to marry a pig.");
   _insults.push_back("Every word you say to me is stupid.");
   _insults.push_back("My last fight ended with my hands covered in blood.");
   _insults.push_back("You are a pain in the backside, sir!");
   _insults.push_back("My sword is famous all over the Caribbean!");
   _insults.push_back("There are no clever moves that can help you now.");
   _insults.push_back("I will milk every drop of blood from your body!");
   _insults.push_back("I hope you have a boat ready for a quick escape.");
   _insults.push_back("My name is feared in every dirty corner of this island!");

   shootAgain();
}


// neo-dimension.wikidot.com/somi:insult-swordfighting

void ProtoBotInsults::shootAgain()
{
   Timer::singleShot(20000 + Random::bounded(60000), [this]() { insult(); });
}



void ProtoBotInsults::insult()
{
   if (!_insults.empty())
   {
      sendMessageSignal(_insults[Random::bounded(_insults.size() - 1)]);
      shootAgain();
   }
}


