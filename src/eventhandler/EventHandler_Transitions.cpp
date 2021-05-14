#include "EventHandler.h"

#include "../plugin-macros.generated.h"

void EventHandler::HandleTransitionCreated(obs_source_t *source)
{
	json eventData;
	eventData["transitionName"] = obs_source_get_name(source);
	eventData["transitionKind"] = obs_source_get_id(source);
	eventData["transitionFixed"] = obs_transition_fixed(source);
	_webSocketServer->BroadcastEvent(EventSubscription::Transitions, "TransitionCreated", eventData);
}

void EventHandler::HandleTransitionRemoved(obs_source_t *source)
{
	json eventData;
	eventData["transitionName"] = obs_source_get_name(source);
	_webSocketServer->BroadcastEvent(EventSubscription::Transitions, "TransitionRemoved", eventData);
}

void EventHandler::HandleTransitionNameChanged(obs_source_t *source, std::string oldTransitionName, std::string transitionName)
{
	json eventData;
	eventData["oldTransitionName"] = oldTransitionName;
	eventData["transitionName"] = transitionName;
	_webSocketServer->BroadcastEvent(EventSubscription::Transitions, "TransitionNameChanged", eventData);
}
