// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;

namespace FlaxEngine
{
    /// <summary>
    /// Contains a collision information passed to the OnCollisionEnter/OnCollisionExit events.
    /// </summary>
    unsafe partial struct Collision : IEnumerable<ContactPoint>
    {
        private class ContactsEnumerator : IEnumerator<ContactPoint>
        {
            private Collision _c;
            private int _index;

            public ContactsEnumerator(ref Collision c)
            {
                _c = c;
                _index = 0;
            }

            public bool MoveNext()
            {
                if (_index == _c.ContactsCount - 1)
                    return false;
                _index++;
                return true;
            }

            public void Reset()
            {
                _index = 0;
            }

            ContactPoint IEnumerator<ContactPoint>.Current
            {
                get
                {
                    if (_index == _c.ContactsCount)
                        throw new InvalidOperationException("Enumeration ended.");
                    fixed (ContactPoint* ptr = &_c.Contacts0)
                        return ptr[_index];
                }
            }

            public object Current
            {
                get
                {
                    if (_index == _c.ContactsCount)
                        throw new InvalidOperationException("Enumeration ended.");
                    fixed (ContactPoint* ptr = &_c.Contacts0)
                        return ptr[_index];
                }
            }

            public void Dispose()
            {
                _c = new Collision();
            }
        }

        /// <summary>
        /// The contacts locations.
        /// </summary>
        /// <remarks>
        /// This property allocates an array of contact points.
        /// </remarks>
        public ContactPoint[] Contacts
        {
            get
            {
                var result = new ContactPoint[ContactsCount];
                fixed (ContactPoint* ptr = &Contacts0)
                {
                    for (int i = 0; i < ContactsCount; i++)
                        result[i] = ptr[i];
                }
                return result;
            }
        }

        /// <summary>
        /// The relative linear velocity of the two colliding objects.
        /// </summary>
        /// <remarks>
        /// Can be used to detect stronger collisions.
        /// </remarks>
        public Vector3 RelativeVelocity
        {
            get
            {
                Vector3.Subtract(ref ThisVelocity, ref OtherVelocity, out var result);
                return result;
            }
        }

        /// <summary>
        /// The first collider (this instance). It may be null if this actor is not the <see cref="Collider"/> (eg. <see cref="Terrain"/>).
        /// </summary>
        public Collider ThisCollider => ThisActor as Collider;

        /// <summary>
        /// The second collider (other instance). It may be null if this actor is not the <see cref="Collider"/> (eg. <see cref="Terrain"/>).
        /// </summary>
        public Collider OtherCollider => OtherActor as Collider;

        /// <inheritdoc />
        IEnumerator<ContactPoint> IEnumerable<ContactPoint>.GetEnumerator()
        {
            return new ContactsEnumerator(ref this);
        }

        /// <inheritdoc />
        IEnumerator IEnumerable.GetEnumerator()
        {
            return new ContactsEnumerator(ref this);
        }
    }
}
