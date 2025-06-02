using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine.GUI;

#pragma warning disable 1591
namespace FlaxEditor.InputConfig
{
    /// <summary>
    /// The input actions processing helper that handles input bindings configuration layer.
    /// </summary>
    public class InputBindingList
    {
        public List<InputBinding> List = new List<InputBinding>();

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

        public InputBinding? Get(InputBinding binding)
        {
            foreach (var item in List)
            {
                if (item.ToString().Equals(binding.ToString()))
                {
                    return item;
                }
            }

            return null; // Or throw, or handle however you want if not found
        }

        public void Add(InputBinding binding)
        {
            if (!List.Contains(binding))
            {
                InputBinding newBinding = new InputBinding(binding.ToString());
                List.Add(newBinding);
            }
        }

        public void Add(InputBinding binding, Action callback)
        {
            if (!List.Contains(binding))
            {
                InputBinding newBinding = new InputBinding(binding.ToString());
                newBinding.Callback = callback;
                List.Add(newBinding);
                return;
            }

            var foundBinding = List.FirstOrDefault(b => b == binding);
            foundBinding.Callback = callback;
        }

#nullable enable annotations
        public void Add(InputBinding binding, Action callback, Action? clear)
        {
            if (!List.Contains(binding))
            {
                InputBinding newBinding = new InputBinding(binding.ToString());
                newBinding.Callback = callback;
                newBinding.Clear = clear;
                List.Add(newBinding);
                return;
            }

            var foundBinding = List.FirstOrDefault(b => b == binding);
            foundBinding.Callback = callback;
        }

        public void Add(params (InputBinding binding, Action callback)[] bindings)
        {
            foreach (var (binding, callback) in bindings)
            {
                this.Add(binding, callback);
            }
        }

#nullable enable annotations
        public void Add(List<(InputBinding, Action, Action?)> bindings)
        {
            foreach (var (binding, callback, clear) in bindings)
            {
                this.Add(binding, callback, clear);
            }
        }

        public InputBindingList(params (InputBinding binding, Action callback)[] bindings)
        {
            foreach (var (binding, callback) in bindings)
            {
                this.Add(binding, callback);
            }
        }

        public void SetCallback(params (InputBinding match, Action callback)[] bindings)
        {
            foreach (var (match, callback) in bindings)
            {
                var binding = List.FirstOrDefault(b => b == match);
                if (binding != null)
                    binding.Callback = callback;
            }
        }


        /// <summary>
        /// Processes the specified key input and tries to invoke first matching callback for the current user input state.
        /// </summary>
        /// <param name="control">The input providing control.</param>
        /// <returns>The InputBinding if event has been handled, otherwise null.</returns>
        public InputBinding? Process(Control control)
        {
            InputBinding? bestMatch = null;
            int maxTriggerCount = -1;

            for (int i = 0; i < List.Count; i++)
            {
                if (List[i].Process(control, false))
                {
                    int triggerCount = List[i].InputTriggers.Count;
                    if (triggerCount > maxTriggerCount)
                    {
                        maxTriggerCount = triggerCount;
                        bestMatch = List[i];
                    }
                }
            }

            bestMatch?.Process(control);
            return bestMatch;
        }

        /// <summary>
        /// Invokes a specific binding.
        /// </summary>
        /// <param name="binding">The binding to execute.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        public bool Invoke(InputBinding binding)
        {
            if (binding == new InputBinding())
                return false;
            for (int i = 0; i < List.Count; i++)
            {
                if (List[i] == binding)
                {
                    List[i].Callback();
                    return true;
                }
            }
            return false;
        }
    }
}
