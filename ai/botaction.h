#ifndef BOTACTION_H
#define BOTACTION_H

class BotAction
{
public:
   //! action types
   enum class ActionType
   {
      ActionIdle,
      ActionWalk,
      ActionBomb
   };

   //! constructor
   explicit BotAction(ActionType action_type = ActionType::ActionIdle);

   //! destructor
   virtual ~BotAction() = default;

   //! getter for action type
   ActionType getActionType() const;

protected:
   //! action type
   ActionType _action_type = ActionType::ActionIdle;
};

#endif  // BOTACTION_H
