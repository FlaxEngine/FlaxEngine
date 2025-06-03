using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Options;
using FlaxEngine.GUI;

#pragma warning disable 1591
namespace FlaxEditor.InputConfig
{
    public struct BindingAction : IEquatable<BindingAction>
    {
        public InputOptionName Name;
        public Action Callback;
        public Action Clear;

        public void ClearState()
        {
            Clear?.Invoke();
        }

        public BindingAction(InputOptionName name, Action callback, Action clear)
        {
            Name = name;
            Callback = callback;
            Clear = clear;
        }
        public BindingAction(InputOptionName name, Action callback)
        {
            Name = name;
            Callback = callback;
        }

        public bool Equals(BindingAction other)
        {
            return Name == other.Name;
        }

        public override bool Equals(object? obj)
        {
            return obj is BindingAction other && Equals(other);
        }

        public override int GetHashCode()
        {
            return Name.GetHashCode();
        }

        public static bool operator ==(BindingAction left, BindingAction right) => left.Equals(right);
        public static bool operator !=(BindingAction left, BindingAction right) => !left.Equals(right);
    }
    /// <summary>
    /// The input actions processing helper that handles input bindings configuration layer.
    /// </summary>
    public class InputBindingList
    {


        public List<BindingAction> List = new List<BindingAction>();

        public void Remove(Action callback)
        {
            for (int i = List.Count - 1; i >= 0; i--)
            {
                if (List[i].Callback == callback)
                {
                    List.RemoveAt(i);
                }
            }
        }

        public void Add(BindingAction bindingAction)
        {
            if (List.Contains(bindingAction))
            {
                List.Remove(bindingAction);
            }
            List.Add(bindingAction);
            return;
        }

        public void Add(BindingAction[] bindings)
        {
            foreach (var binding in bindings)
            {
                this.Add(binding);
            }
        }

        public InputBindingList(BindingAction[] bindings)
        {
            foreach (var binding in bindings)
            {
                this.Add(binding);
            }
        }

        /// <summary>
        /// Processes the specified key input and tries to invoke first matching callback for the current user input state.
        /// </summary>
        /// <param name="control">The input providing control.</param>
        /// <returns>The InputBinding if event has been handled, otherwise null.</returns>
        public BindingAction? Process(Control control)
        {
            BindingAction? bestMatch = null;
            int maxTriggerCount = -1;

            foreach (var action in List)
            {
                var binding = InputOptions.Dictionary[action.Name];
                if (binding.Process(control))
                {
                    int triggerCount = binding.InputTriggers.Count;
                    if (triggerCount > maxTriggerCount)
                    {
                        maxTriggerCount = triggerCount;
                        bestMatch = action;
                    }
                }
            }

            bestMatch?.Callback.Invoke();
            return bestMatch;
        }

        /// <summary>
        /// Invokes a specific binding.
        /// </summary>
        /// <param InputOptionName="name">The binding to execute.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        public bool Invoke(InputOptionName name)
        {
            foreach (var action in List)
            {
                if (action.Name == name)
                {
                    action.Callback?.Invoke();
                    return true;
                }
            }
            return false;
        }
    }
}
