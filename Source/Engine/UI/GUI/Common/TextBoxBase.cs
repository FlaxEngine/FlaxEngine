// Copyright (c) Wojciech Figat. All rights reserved.

using System;
#if FLAX_EDITOR
using FlaxEditor.Options;
#endif
using FlaxEngine.Assertions;
using FlaxEngine.Utilities;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for all text box controls which can gather text input from the user.
    /// </summary>
    public abstract class TextBoxBase : ContainerControl
    {
        /// <summary>
        /// The delete control character (used for text filtering).
        /// </summary>
        protected const char DelChar = (char)0x7F;

        /// <summary>
        /// The text separators (used for words skipping).
        /// </summary>
        protected static readonly char[] Separators =
        {
            ' ',
            '.',
            ',',
            '\t',
            '\r',
            '\n',
            ':',
            ';',
            '\'',
            '\"',
            ')',
            '(',
            '/',
            '\\',
            '>',
            '<',
        };

        /// <summary>
        /// Default height of the text box
        /// </summary>
        public static float DefaultHeight = 18;

        /// <summary>
        /// Left and right margin for text inside the text box bounds rectangle
        /// </summary>
        public static float DefaultMargin = 4;

        /// <summary>
        /// The current text value.
        /// </summary>
        protected string _text = string.Empty;

        /// <summary>
        /// The text value captured when user started editing text. Used to detect content modification.
        /// </summary>
        protected string _onStartEditValue;

        /// <summary>
        /// Flag used to indicate whenever user is editing the text.
        /// </summary>
        protected bool _isEditing;

        /// <summary>
        /// The view offset
        /// </summary>
        protected Float2 _viewOffset;

        /// <summary>
        /// The target view offset.
        /// </summary>
        protected Float2 _targetViewOffset;

        /// <summary>
        /// The text size calculated from font.
        /// </summary>
        protected Float2 _textSize;

        /// <summary>
        /// Flag used to indicate whenever text can contain multiple lines.
        /// </summary>
        protected bool _isMultiline;

        /// <summary>
        /// Flag used to indicate whenever text is read-only and cannot be modified by the user.
        /// </summary>
        protected bool _isReadOnly;

        /// <summary>
        /// Flag used to indicate whenever text is selectable.
        /// </summary>
        protected bool _isSelectable = true;

        /// <summary>
        /// The maximum length of the text.
        /// </summary>
        protected int _maxLength;

        /// <summary>
        /// Flag used to indicate whenever user is selecting text.
        /// </summary>
        protected bool _isSelecting;

        /// <summary>
        /// The selection start position (character index).
        /// </summary>
        protected int _selectionStart;

        /// <summary>
        /// The selection end position (character index).
        /// </summary>
        protected int _selectionEnd;

        /// <summary>
        /// The animate time for selection.
        /// </summary>
        protected float _animateTime;

        /// <summary>
        /// If the cursor should change to an IBeam
        /// </summary>
        protected bool _changeCursor = true;

        /// <summary>
        /// True if always return true as default for key events, otherwise won't consume them.
        /// </summary>
        protected bool _consumeAllKeyDownEvents = true;

        /// <summary>
        /// Event fired when text gets changed
        /// </summary>
        public event Action TextChanged;

        /// <summary>
        /// Event fired when text gets changed after editing (user accepted entered value).
        /// </summary>
        public event Action EditEnd;

        /// <summary>
        /// Event fired when text gets changed after editing (user accepted entered value).
        /// </summary>
        public event Action<TextBoxBase> TextBoxEditEnd;

        /// <summary>
        /// Event fired when a key is down.
        /// </summary>
        public event Action<KeyboardKeys> KeyDown;

        /// <summary>
        /// Event fired when a key is up.
        /// </summary>
        public event Action<KeyboardKeys> KeyUp;

        /// <summary>
        /// Gets or sets a value indicating whether the text box can end edit via left click outside of the control
        /// </summary>
        [HideInEditor]
        public bool EndEditOnClick { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether this is a multiline text box control.
        /// </summary>
        [EditorOrder(40), Tooltip("If checked, the textbox will support multiline text input.")]
        public bool IsMultiline
        {
            get => _isMultiline;
            set
            {
                if (_isMultiline != value)
                {
                    _isMultiline = value;

                    OnIsMultilineChanged();
                    Deselect();

                    if (!_isMultiline)
                    {
                        var lines = _text.Split('\n');
                        _text = lines[0];
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the maximum number of characters the user can type into the text box control.
        /// </summary>
        [EditorOrder(50), Tooltip("The maximum number of characters the user can type into the text box control.")]
        public int MaxLength
        {
            get => _maxLength;
            set
            {
                if (_maxLength <= 0)
                    throw new ArgumentOutOfRangeException(nameof(MaxLength));

                if (_maxLength != value)
                {
                    _maxLength = value;

                    // Cut too long text
                    if (_text.Length > _maxLength)
                    {
                        Text = _text.Substring(0, _maxLength);
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether text in the text box is read-only. 
        /// </summary>
        [EditorOrder(60), Tooltip("If checked, text in the text box is read-only.")]
        public bool IsReadOnly
        {
            get => _isReadOnly;
            set
            {
                if (_isReadOnly != value)
                {
                    _isReadOnly = value;
                    OnIsReadOnlyChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether text can be selected in text box. 
        /// </summary>
        [EditorOrder(62), Tooltip("If checked, text can be selected in text box.")]
        public bool IsSelectable
        {
            get => _isSelectable;
            set
            {
                if (_isSelectable != value)
                {
                    _isSelectable = value;
                    OnIsSelectableChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether apply clipping mask on text during rendering.
        /// </summary>
        [EditorOrder(529)]
        public bool ClipText { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether you can scroll the text in the text box (eg. with a mouse wheel).
        /// </summary>
        [EditorOrder(41)]
        public bool IsMultilineScrollable { get; set; } = true;

        /// <summary>
        /// Gets or sets textbox background color when the control is selected (has focus).
        /// </summary>
        [EditorDisplay("Background Style"), EditorOrder(2001), Tooltip("The textbox background color when the control is selected (has focus)."), ExpandGroups]
        public Color BackgroundSelectedColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the caret (Transparent if not used).
        /// </summary>
        [EditorDisplay("Caret Style"), EditorOrder(2020), Tooltip("The color of the caret (Transparent if not used)."), ExpandGroups]
        public Color CaretColor { get; set; }

        /// <summary>
        /// Gets or sets the speed of the caret flashing animation.
        /// </summary>
        [EditorDisplay("Caret Style"), EditorOrder(2021), Tooltip("The speed of the caret flashing animation.")]
        public float CaretFlashSpeed { get; set; } = 6.0f;

        /// <summary>
        /// Gets or sets the speed of the selection background flashing animation.
        /// </summary>
        [EditorDisplay("Background Style"), EditorOrder(2002), Tooltip("The speed of the selection background flashing animation.")]
        public float BackgroundSelectedFlashSpeed { get; set; } = 6.0f;

        /// <summary>
        /// Gets or sets whether to have a border.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2010), Tooltip("Whether to have a border."), ExpandGroups]
        public bool HasBorder { get; set; } = true;

        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2011), Tooltip("The thickness of the border."), Limit(0)]
        public float BorderThickness { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border (Transparent if not used).
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2012), Tooltip("The color of the border (Transparent if not used).")]
        public Color BorderColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the border when control is focused (Transparent if not used).
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2013), Tooltip("The color of the border when control is focused (Transparent if not used)")]
        public Color BorderSelectedColor { get; set; }

        /// <summary>
        /// Gets the size of the text (cached).
        /// </summary>
        public Float2 TextSize => _textSize;

        /// <summary>
        /// Occurs when target view offset gets changed.
        /// </summary>
        public event Action TargetViewOffsetChanged;

        /// <summary>
        /// Gets the current view offset (text scrolling offset). Includes the smoothing.
        /// </summary>
        public Float2 ViewOffset => _viewOffset;

        /// <summary>
        /// Gets or sets the target view offset (text scrolling offset).
        /// </summary>
        [NoAnimate, NoSerialize, HideInEditor]
        public Float2 TargetViewOffset
        {
            get => _targetViewOffset;
            set
            {
                value = Float2.Round(value);
                if (Float2.NearEqual(ref value, ref _targetViewOffset))
                    return;
                _targetViewOffset = _viewOffset = value;
                OnTargetViewOffsetChanged();
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is editing the text.
        /// </summary>
        public bool IsEditing => _isEditing;

        /// <summary>
        /// Gets or sets text property.
        /// </summary>
        [EditorOrder(0), MultilineText, Tooltip("The entered text.")]
        public string Text
        {
            get => _text;
            set
            {
                // Skip set if user is editing value
                if (_isEditing)
                    return;

                SetText(value);
            }
        }

        /// <summary>
        /// Sets the text (forced, even if user is editing it).
        /// </summary>
        /// <param name="value">The value.</param>
        [NoAnimate]
        public void SetText(string value)
        {
            // Prevent from null problems
            if (value == null)
                value = string.Empty;

            // Filter text
            if (value.IndexOf('\r') != -1)
                value = value.Replace("\r", "");

            // Filter text (handle backspace control character)
            if (value.IndexOf(DelChar) != -1)
                value = value.Replace(DelChar.ToString(), "");

            // Clamp length
            if (value.Length > MaxLength)
                value = value.Substring(0, MaxLength);

            // Ensure to use only single line
            if (_isMultiline == false && value.Length > 0)
            {
                // Extract only the first line
                value = value.GetLines()[0];
            }

            if (_text != value)
            {
                Deselect();
                ResetViewOffset();

                _text = value;

                OnTextChanged();
            }
        }

        /// <summary>
        /// Sets the text as it was entered by user (focus, change value, defocus).
        /// </summary>
        /// <param name="value">The value.</param>
        [NoAnimate]
        public void SetTextAsUser(string value)
        {
            Focus();
            SetText(value);
            RemoveFocus();
        }

        /// <summary>
        /// Gets length of the text
        /// </summary>
        public int TextLength => _text.Length;

        /// <summary>
        /// Gets the currently selected text in the control.
        /// </summary>
        public string SelectedText
        {
            get
            {
                int length = SelectionLength;
                return length > 0 ? _text.Substring(SelectionLeft, length) : string.Empty;
            }
        }

        /// <summary>
        /// Gets the number of characters selected in the text box.
        /// </summary>
        public int SelectionLength => Mathf.Abs(_selectionEnd - _selectionStart);

        /// <summary>
        /// Gets or sets the selection range.
        /// </summary>
        [EditorOrder(50)]
        public TextRange SelectionRange
        {
            get => new TextRange(SelectionLeft, SelectionRight);
            set => SetSelection(value.StartIndex, value.EndIndex, false);
        }

        /// <summary>
        /// Returns true if any text is selected, otherwise false
        /// </summary>
        public bool HasSelection => SelectionLength > 0;

        /// <summary>
        /// Index of the character on left edge of the selection
        /// </summary>
        protected int SelectionLeft => Mathf.Min(_selectionStart, _selectionEnd);

        /// <summary>
        /// Index of the character on right edge of the selection
        /// </summary>
        protected int SelectionRight => Mathf.Max(_selectionStart, _selectionEnd);

        /// <summary>
        /// Gets current caret position (index of the character)
        /// </summary>
        protected int CaretPosition => _selectionEnd;

        /// <summary>
        /// Calculates the caret rectangle.
        /// </summary>
        protected Rectangle CaretBounds
        {
            get
            {
                const float caretWidth = 1.2f;
                Float2 caretPos = GetCharPosition(CaretPosition, out var height);
                return new Rectangle(
                                     caretPos.X - (caretWidth * 0.5f),
                                     caretPos.Y,
                                     caretWidth,
                                     height * DpiScale);
            }
        }

        /// <summary>
        /// Gets rectangle with area for text
        /// </summary>
        protected virtual Rectangle TextRectangle => new Rectangle(DefaultMargin, 1, Width - 2 * DefaultMargin, Height - 2);

        /// <summary>
        /// Gets rectangle used to clip text
        /// </summary>
        protected virtual Rectangle TextClipRectangle => new Rectangle(1, 1, Width - 2, Height - 2);

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBoxBase"/> class.
        /// </summary>
        protected TextBoxBase()
        : this(false, 0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBoxBase"/> class.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support.</param>
        /// <param name="x">The control position X coordinate.</param>
        /// <param name="y">The control position Y coordinate.</param>
        /// <param name="width">The control width.</param>
        protected TextBoxBase(bool isMultiline, float x, float y, float width = 120)
        : base(x, y, width, DefaultHeight)
        {
            _isMultiline = isMultiline;
            _maxLength = 2147483646;
            _selectionStart = _selectionEnd = -1;

            var style = Style.Current;
            CaretColor = style.Foreground;
            BorderColor = Color.Transparent;
            BorderSelectedColor = style.BackgroundSelected;
            BackgroundColor = style.TextBoxBackground;
            BackgroundSelectedColor = style.TextBoxBackgroundSelected;
        }

        /// <summary>
        /// Clears all text from the text box control. 
        /// </summary>
        public virtual void Clear()
        {
            Text = string.Empty;
        }

        /// <summary>
        /// Clear selection range
        /// </summary>
        public virtual void ClearSelection()
        {
            OnSelectingEnd();
            SetSelection(-1);
        }

        /// <summary>
        /// Resets the view offset (text scroll view).
        /// </summary>
        public virtual void ResetViewOffset()
        {
            TargetViewOffset = Float2.Zero;
        }

        /// <summary>
        /// Called when target view offset gets changed.
        /// </summary>
        protected virtual void OnTargetViewOffsetChanged()
        {
            TargetViewOffsetChanged?.Invoke();
        }

        /// <summary>
        /// Copies the current selection in the text box to the Clipboard. 
        /// </summary>
        public virtual void Copy()
        {
            var selectedText = SelectedText;
            if (selectedText.Length > 0)
            {
                // Copy selected text
                Clipboard.Text = selectedText;
            }
        }

        /// <summary>
        /// Moves the current selection in the text box to the Clipboard. 
        /// </summary>
        public virtual void Cut()
        {
            var selectedText = SelectedText;
            if (selectedText.Length > 0)
            {
                // Copy selected text
                Clipboard.Text = selectedText;

                if (IsReadOnly)
                    return;

                // Remove selection
                int left = SelectionLeft;
                _text = _text.Remove(left, SelectionLength);
                SetSelection(left);
                OnTextChanged();
            }
        }

        /// <summary>
        /// Replaces the current selection in the text box with the contents of the Clipboard.
        /// </summary>
        public virtual void Paste()
        {
            if (IsReadOnly)
                return;

            // Get clipboard data
            var clipboardText = Clipboard.Text;
            if (string.IsNullOrEmpty(clipboardText))
                return;

            Insert(clipboardText);
        }

        /// <summary>
        /// Duplicates the current selection in the text box.
        /// </summary>
        public virtual void Duplicate()
        {
            if (IsReadOnly)
                return;

            var selectedText = SelectedText;
            if (selectedText.Length > 0)
            {
                var right = SelectionRight;
                SetSelection(right);
                Insert(selectedText);
                SetSelection(right, right + selectedText.Length);
            }
        }

        /// <summary>
        /// Ensures that the caret is visible in the TextBox window, by scrolling the TextBox control surface if necessary.
        /// </summary>
        public virtual void ScrollToCaret()
        {
            // If it's empty
            if (_text.Length == 0)
            {
                TargetViewOffset = Float2.Zero;
                return;
            }

            // If it's not selected
            if (_selectionStart == -1 && _selectionEnd == -1)
            {
                return;
            }

            Rectangle caretBounds = CaretBounds;
            Rectangle textArea = TextRectangle;

            // Update view offset (caret needs to be in a view)
            var caretInView = caretBounds.Location - _targetViewOffset;
            var clampedCaretInView = Float2.Clamp(caretInView, textArea.UpperLeft, textArea.BottomRight);
            TargetViewOffset += caretInView - clampedCaretInView;
        }

        /// <summary>
        /// Selects all text in the text box.
        /// </summary>
        public virtual void SelectAll()
        {
            if (TextLength > 0)
            {
                SetSelection(0, TextLength);
            }
        }

        /// <summary>
        /// Sets the selection to empty value.
        /// </summary>
        public virtual void Deselect()
        {
            SetSelection(-1);
        }

        /// <summary>
        /// Gets the character the index at point (eg. mouse location in control-space).
        /// </summary>
        /// <param name="location">The location (in control-space).</param>
        /// <returns>The character index under the location</returns>
        public virtual int CharIndexAtPoint(ref Float2 location)
        {
            return HitTestText(location + _viewOffset);
        }

        /// <summary>
        /// Inserts the specified character (at the current selection).
        /// </summary>
        /// <param name="c">The character.</param>
        public virtual void Insert(char c)
        {
            Insert(c.ToString());
        }

        /// <summary>
        /// Inserts the specified text (at the current selection).
        /// </summary>
        /// <param name="str">The string.</param>
        public virtual void Insert(string str)
        {
            if (IsReadOnly)
                return;

            // Filter text
            if (str.IndexOf('\r') != -1)
                str = str.Replace("\r", "");
            if (str.IndexOf(DelChar) != -1)
                str = str.Replace(DelChar.ToString(), "");
            if (!IsMultiline && str.IndexOf('\n') != -1)
                str = str.Replace("\n", "");

            int selectionLength = SelectionLength;
            int charactersLeft = MaxLength - _text.Length + selectionLength;
            Assert.IsTrue(charactersLeft >= 0);
            if (charactersLeft == 0)
                return;
            if (charactersLeft < str.Length)
                str = str.Substring(0, charactersLeft);

            if (TextLength == 0)
            {
                _text = str;
                SetSelection(TextLength);
            }
            else
            {
                var left = SelectionLeft >= 0 ? SelectionLeft : 0;
                if (HasSelection)
                {
                    _text = _text.Remove(left, selectionLength);
                    SetSelection(left);
                }
                _text = _text.Insert(left, str);

                SetSelection(left + str.Length);
            }

            OnTextChanged();
        }

        /// <summary>
        /// Moves the caret right.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveRight(bool shift, bool ctrl)
        {
            if (HasSelection && !shift)
            {
                SetSelection(SelectionRight);
            }
            else if (SelectionRight < TextLength || (_selectionEnd < _selectionStart && _selectionStart == TextLength))
            {
                int position;
                if (ctrl)
                    position = FindNextWordBegin();
                else
                    position = _selectionEnd + 1;

                if (shift)
                {
                    SetSelection(_selectionStart, position);
                }
                else
                {
                    SetSelection(position);
                }
            }
        }

        /// <summary>
        /// Moves the caret left.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveLeft(bool shift, bool ctrl)
        {
            if (HasSelection && !shift)
            {
                SetSelection(SelectionLeft);
            }
            else if (SelectionLeft >= 0)
            {
                int position;
                if (ctrl)
                    position = FindPrevWordBegin();
                else
                    position = Mathf.Max(_selectionEnd - 1, 0);

                if (shift)
                {
                    SetSelection(_selectionStart, position);
                }
                else
                {
                    SetSelection(position);
                }
            }
        }

        /// <summary>
        /// Moves the caret down.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveDown(bool shift, bool ctrl)
        {
            if (HasSelection && !shift)
            {
                SetSelection(SelectionRight);
            }
            else
            {
                int position = FindLineDownChar(CaretPosition);

                if (shift)
                {
                    SetSelection(_selectionStart, position);
                }
                else
                {
                    SetSelection(position);
                }
            }
        }

        /// <summary>
        /// Moves the caret up.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveUp(bool shift, bool ctrl)
        {
            if (HasSelection && !shift)
            {
                SetSelection(SelectionLeft);
            }
            else
            {
                int position = FindLineUpChar(CaretPosition);

                if (shift)
                {
                    SetSelection(_selectionStart, position);
                }
                else
                {
                    SetSelection(position);
                }
            }
        }

        /// <summary>
        /// Sets the caret position.
        /// </summary>
        /// <param name="caret">The caret position.</param>
        /// <param name="withScroll">If set to <c>true</c> with auto-scroll.</param>
        protected virtual void SetSelection(int caret, bool withScroll = true)
        {
            SetSelection(caret, caret);
        }

        /// <summary>
        /// Sets the selection.
        /// </summary>
        /// <param name="start">The selection start character.</param>
        /// <param name="end">The selection end character.</param>
        /// <param name="withScroll">If set to <c>true</c> with auto-scroll.</param>
        protected virtual void SetSelection(int start, int end, bool withScroll = true)
        {
            // Update parameters
            int textLength = _text.Length;
            _selectionStart = Mathf.Clamp(start, -1, textLength);
            _selectionEnd = Mathf.Clamp(end, -1, textLength);

            if (withScroll)
            {
                // Update view on caret modified
                ScrollToCaret();

                // Reset caret and selection animation
                _animateTime = 0.0f;
            }
        }

        private int FindNextWordBegin()
        {
            int textLength = TextLength;
            int caretPos = CaretPosition;

            if (caretPos + 1 >= textLength)
                return textLength;

            int spaceLoc = _text.IndexOfAny(Separators, caretPos + 1);

            if (spaceLoc == -1)
                spaceLoc = textLength;
            else
                spaceLoc++;

            return spaceLoc;
        }

        private int FindPrevWordBegin()
        {
            int caretPos = CaretPosition;

            if (caretPos - 2 < 0)
                return 0;

            int spaceLoc = _text.LastIndexOfAny(Separators, caretPos - 2);

            if (spaceLoc == -1)
                spaceLoc = 0;
            else
                spaceLoc++;

            return spaceLoc;
        }

        private int FindPrevLineBegin()
        {
            int caretPos = CaretPosition;
            if (caretPos - 2 < 0)
                return 0;
            int newLineLoc = _text.LastIndexOf('\n', caretPos - 2);
            if (newLineLoc == -1)
                newLineLoc = 0;
            else
                newLineLoc++;
            return newLineLoc;
        }

        private int FindNextLineBegin()
        {
            int caretPos = CaretPosition;
            if (caretPos + 2 > TextLength)
                return TextLength;
            int newLineLoc = _text.IndexOf('\n', caretPos + 2);
            if (newLineLoc == -1)
                newLineLoc = TextLength;
            else
                newLineLoc++;
            return newLineLoc;
        }

        private int FindLineDownChar(int index)
        {
            if (!IsMultiline)
                return 0;

            var location = GetCharPosition(index, out var height);
            location.Y += height;

            return HitTestText(location);
        }

        private int FindLineUpChar(int index)
        {
            if (!IsMultiline)
                return _text.Length;

            var location = GetCharPosition(index, out var height);
            location.Y -= height;

            return HitTestText(location);
        }

        private void RemoveFocus()
        {
            if (Parent != null)
                Parent.Focus();
            else
                Defocus();
        }

        /// <summary>
        /// Calculates total text size. Called by <see cref="OnTextChanged"/> to cache the text size.
        /// </summary>
        /// <returns>The total text size.</returns>
        public abstract Float2 GetTextSize();

        /// <summary>
        /// Calculates character position for given character index.
        /// </summary>
        /// <param name="index">The text position to get it's coordinates.</param>
        /// <param name="height">The character height (at the given character position).</param>
        /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
        public abstract Float2 GetCharPosition(int index, out float height);

        /// <summary>
        /// Calculates hit character index at given location.
        /// </summary>
        /// <param name="location">The location to test.</param>
        /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
        public abstract int HitTestText(Float2 location);

        /// <summary>
        /// Called when is multiline gets changed.
        /// </summary>
        protected virtual void OnIsMultilineChanged()
        {
        }

        /// <summary>
        /// Called when is read only gets changed.
        /// </summary>
        protected virtual void OnIsReadOnlyChanged()
        {
        }

        /// <summary>
        /// Called when is selectable flag gets changed.
        /// </summary>
        protected virtual void OnIsSelectableChanged()
        {
        }

        /// <summary>
        /// Action called when user starts text selecting
        /// </summary>
        protected virtual void OnSelectingBegin()
        {
            if (!_isSelecting)
            {
                // Set flag
                _isSelecting = true;

                // Start tracking mouse
                StartMouseCapture();
            }
        }

        /// <summary>
        /// Action called when user ends text selecting
        /// </summary>
        protected virtual void OnSelectingEnd()
        {
            if (_isSelecting)
            {
                // Clear flag
                _isSelecting = false;

                // Stop tracking mouse
                EndMouseCapture();
            }
        }

        /// <summary>
        /// Action called when user starts text editing
        /// </summary>
        protected virtual void OnEditBegin()
        {
            if (_isEditing)
                return;

            _isEditing = true;
            _onStartEditValue = _text;

            // Reset caret visibility
            _animateTime = 0;
        }

        /// <summary>
        /// Action called when user ends text editing.
        /// </summary>
        protected virtual void OnEditEnd()
        {
            if (!_isEditing)
                return;

            _isEditing = false;
            if (_onStartEditValue != _text)
            {
                _onStartEditValue = _text;
                EditEnd?.Invoke();
                TextBoxEditEnd?.Invoke(this);
            }
            _onStartEditValue = string.Empty;

            ClearSelection();
            ResetViewOffset();
        }

        /// <summary>
        /// Action called when text gets modified.
        /// </summary>
        protected virtual void OnTextChanged()
        {
            _textSize = GetTextSize();
            TextChanged?.Invoke();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            bool isDeltaSlow = deltaTime > (1 / 20.0f);

            _animateTime += deltaTime;

            // Animate view offset
            _viewOffset = isDeltaSlow ? _targetViewOffset : Float2.Lerp(_viewOffset, _targetViewOffset, deltaTime * 20.0f);

            // Clicking outside of the text box will end text editing. Left will keep the value, right will restore original value
            if (_isEditing && EndEditOnClick)
            {
                if (!IsMouseOver && RootWindow.ContainsFocus)
                {
                    if (Input.GetMouseButtonDown(MouseButton.Left))
                    {
                        RemoveFocus();
                    }
                    else if (Input.GetMouseButtonDown(MouseButton.Right))
                    {
                        RestoreTextFromStart();
                        RemoveFocus();
                    }
                }
            }

            base.Update(deltaTime);
        }

        /// <summary>
        /// Restores the Text from the start.
        /// </summary>
        public void RestoreTextFromStart()
        {
            // Restore text from start
            SetSelection(-1);
            _text = _onStartEditValue;
            OnTextChanged();
        }

        /// <inheritdoc />
        public override void OnGotFocus()
        {
            base.OnGotFocus();

            if (IsReadOnly)
                return;
            OnEditBegin();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();

            if (IsReadOnly)
                return;
            OnEditEnd();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Clear flag
            _isSelecting = false;
        }

        /// <inheritdoc />
        public override void NavigationFocus()
        {
            base.NavigationFocus();

            if (IsNavFocused)
                SelectAll();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            OnEditEnd();
            if (IsNavFocused)
            {
                OnEditBegin();
                SelectAll();
            }

            base.OnSubmit();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            if (_isEditing && _changeCursor)
                Cursor = CursorType.IBeam;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (Cursor == CursorType.IBeam)
                Cursor = CursorType.Default;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            if (_isSelecting)
            {
                // Find char index at current mouse location
                int currentIndex = CharIndexAtPoint(ref location);

                // Modify selection end
                SetSelection(_selectionStart, currentIndex);
            }

            if (Cursor == CursorType.Default && _isEditing && _changeCursor)
                Cursor = CursorType.IBeam;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && _isSelectable)
            {
                Focus();
                OnSelectingBegin();

                // Calculate char index under the mouse location
                var hitPos = CharIndexAtPoint(ref location);

                // Select range with shift
                if (_selectionStart != -1 && RootWindow.GetKey(KeyboardKeys.Shift) && SelectionLength == 0)
                {
                    if (hitPos < _selectionStart)
                        SetSelection(hitPos, _selectionStart);
                    else
                        SetSelection(_selectionStart, hitPos);
                }
                else if (string.IsNullOrEmpty(_text))
                {
                    SetSelection(0);
                }
                else
                {
                    SetSelection(hitPos);
                }

                if (Cursor == CursorType.Default && _changeCursor)
                    Cursor = CursorType.IBeam;

                return true;
            }

            if (button == MouseButton.Left && !IsFocused)
            {
                Focus();
                if (_changeCursor)
                    Cursor = CursorType.IBeam;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (button == MouseButton.Left && _isSelectable)
            {
                OnSelectingEnd();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (base.OnMouseWheel(location, delta))
                return true;

            // Multiline scroll
            if (IsMultiline && _text.Length != 0 && IsMultilineScrollable)
            {
                if (Input.GetKey(KeyboardKeys.Shift))
                    TargetViewOffset = Float2.Clamp(_targetViewOffset - new Float2(delta * 20.0f, 0), Float2.Zero, new Float2(_textSize.X, _targetViewOffset.Y));
                else
                    TargetViewOffset = Float2.Clamp(_targetViewOffset - new Float2(0, delta * 10.0f), Float2.Zero, new Float2(_targetViewOffset.X, _textSize.Y - Height));
                
                return true;
            }

            // No event handled
            return false;
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
                return true;
            if (IsReadOnly)
                return false;
            Insert(c);
            return true;
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            base.OnKeyUp(key);
            KeyUp?.Invoke(key);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            var window = Root;
            bool shiftDown = window.GetKey(KeyboardKeys.Shift);
            bool ctrDown = window.GetKey(KeyboardKeys.Control);
            KeyDown?.Invoke(key);

            // Handle controls that have bindings
#if FLAX_EDITOR
            InputOptions options = FlaxEditor.Editor.Instance.Options.Options.Input;
            if (options.Copy.Process(this))
            {
                Copy();
                return true;
            }
            else if (options.Paste.Process(this))
            {
                Paste();
                return true;
            }
            else if (options.Duplicate.Process(this))
            {
                Duplicate();
                return true;
            }
            else if (options.Cut.Process(this))
            {
                Cut();
                return true;
            }
            else if (options.SelectAll.Process(this))
            {
                SelectAll();
                return true;
            }
            else if (options.DeselectAll.Process(this))
            {
                Deselect();
                return true;
            }
#endif

            // Handle controls without bindings
            switch (key)
            {
            case KeyboardKeys.ArrowRight:
                MoveRight(shiftDown, ctrDown);
                return true;
            case KeyboardKeys.ArrowLeft:
                MoveLeft(shiftDown, ctrDown);
                return true;
            case KeyboardKeys.ArrowUp:
                MoveUp(shiftDown, ctrDown);
                return true;
            case KeyboardKeys.ArrowDown:
                MoveDown(shiftDown, ctrDown);
                return true;
            case KeyboardKeys.C:
                if (ctrDown)
                {
                    Copy();
                    return true;
                }
                break;
            case KeyboardKeys.V:
                if (ctrDown)
                {
                    Paste();
                    return true;
                }
                break;
            case KeyboardKeys.D:
                if (ctrDown)
                {
                    Duplicate();
                    return true;
                }
                break;
            case KeyboardKeys.X:
                if (ctrDown)
                {
                    Cut();
                    return true;
                }
                break;
            case KeyboardKeys.A:
                if (ctrDown)
                {
                    SelectAll();
                    return true;
                }
                break;
            case KeyboardKeys.Backspace:
            {
                if (IsReadOnly)
                    return true;

                if (ctrDown)
                {
                    int prevWordBegin = FindPrevWordBegin();
                    _text = _text.Remove(prevWordBegin, CaretPosition - prevWordBegin);
                    SetSelection(prevWordBegin);
                    OnTextChanged();
                    return true;
                }

                int left = SelectionLeft;
                if (HasSelection)
                {
                    _text = _text.Remove(left, SelectionLength);
                    SetSelection(left);
                    OnTextChanged();
                }
                else if (CaretPosition > 0)
                {
                    left -= 1;
                    _text = _text.Remove(left, 1);
                    SetSelection(left);
                    OnTextChanged();
                }

                return true;
            }
            case KeyboardKeys.PageDown:
            {
                if (IsScrollable && IsMultiline)
                {
                    var location = GetCharPosition(_selectionStart, out var height);
                    var sizeHeight = Size.Y / height;
                    location.Y += height * (int)sizeHeight;
                    TargetViewOffset = Vector2.Clamp(new Float2(0, location.Y), Float2.Zero, TextSize - new Float2(0, Size.Y));
                    SetSelection(HitTestText(location));
                }
                return true;
            }
            case KeyboardKeys.PageUp:
            {
                if (IsScrollable && IsMultiline)
                {
                    var location = GetCharPosition(_selectionStart, out var height);
                    var sizeHeight =  Size.Y / height;
                    location.Y -= height * (int)sizeHeight;
                    TargetViewOffset = Vector2.Clamp(new Float2(0, location.Y), Float2.Zero, TextSize - new Float2(0, Size.Y));
                    SetSelection(HitTestText(location));
                }
                return true;
            }
            case KeyboardKeys.Delete:
            {
                if (IsReadOnly)
                    return true;

                int left = SelectionLeft;
                if (HasSelection)
                {
                    _text = _text.Remove(left, SelectionLength);
                    SetSelection(left);
                    OnTextChanged();
                }
                else if (TextLength > 0 && left < TextLength)
                {
                    _text = _text.Remove(left, 1);
                    SetSelection(left);
                    OnTextChanged();
                }

                return true;
            }
            case KeyboardKeys.Escape:
            {
                if (IsReadOnly)
                {
                    SetSelection(_selectionEnd);
                    return true;
                }

                RestoreTextFromStart();

                if (!IsNavFocused)
                    RemoveFocus();

                return true;
            }
            case KeyboardKeys.Return:
                if (IsMultiline)
                {
                    // Insert new line
                    Insert('\n');
                    ScrollToCaret();
                }
                else if (!IsNavFocused)
                {
                    // End editing
                    RemoveFocus();
                }
                else
                    return false;
                return true;
            case KeyboardKeys.Home:
                if (shiftDown)
                {
                    // Select text from the current cursor point back to the beginning of the line
                    if (_selectionStart != -1)
                    {
                        SetSelection(FindPrevLineBegin(), _selectionStart);
                    }
                }
                else
                {
                    // Move caret to the first character
                    SetSelection(0);
                }
                return true;
            case KeyboardKeys.End:
            {
                // Select text from the current cursor point to the beginning of a new line
                if (shiftDown && _selectionStart != -1)
                    SetSelection(_selectionStart, FindNextLineBegin());
                // Move caret after last character
                else
                    SetSelection(TextLength);
                
                return true;
            }
            case KeyboardKeys.Tab:
                // Don't process
                return false;
            }

            return _consumeAllKeyDownEvents;
        }
    }
}
