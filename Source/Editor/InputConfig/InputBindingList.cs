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

        public void Add(InputBinding binding)
        {
            if (!List.Contains(binding))
            {
                List.Add(binding);
            }
        }

        public void Add(InputBinding binding, Action callback)
        {
            if (!List.Contains(binding))
            {
                binding.Callback = callback;
                List.Add(binding);
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
        /// <returns>True if event has been handled, otherwise false.</returns>
        public bool Process(Control control)
        {
            for (int i = 0; i < List.Count; i++)
            {
                if (List[i].Process(control))
                {
                    return true;
                }
            }
            return false;
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
