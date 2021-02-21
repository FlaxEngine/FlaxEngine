// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.Assertions;
using FlaxEngine.Utilities;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for all text box controls which can gather text input from the user.
    /// </summary>
    public abstract class TextBoxBase : Control
    {
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
        protected Vector2 _viewOffset;

        /// <summary>
        /// The target view offset.
        /// </summary>
        protected Vector2 _targetViewOffset;

        /// <summary>
        /// The text size calculated from font.
        /// </summary>
        protected Vector2 _textSize;

        /// <summary>
        /// Flag used to indicate whenever text can contain multiple lines.
        /// </summary>
        protected bool _isMultiline;

        /// <summary>
        /// Flag used to indicate whenever text is read-only and cannot be modified by the user.
        /// </summary>
        protected bool _isReadOnly;

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
        /// Gets or sets textbox background color when the control is selected (has focus).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The textbox background color when the control is selected (has focus).")]
        public Color BackgroundSelectedColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the caret (Transparent if not used).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the caret (Transparent if not used).")]
        public Color CaretColor { get; set; }

        /// <summary>
        /// Gets or sets the speed of the caret flashing animation.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The speed of the caret flashing animation.")]
        public float CaretFlashSpeed { get; set; } = 6.0f;

        /// <summary>
        /// Gets or sets the speed of the selection background flashing animation.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The speed of the selection background flashing animation.")]
        public float BackgroundSelectedFlashSpeed { get; set; } = 6.0f;

        /// <summary>
        /// Gets or sets the color of the border (Transparent if not used).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the border (Transparent if not used).")]
        public Color BorderColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the border when control is focused (Transparent if not used).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the border when control is focused (Transparent if not used)")]
        public Color BorderSelectedColor { get; set; }

        /// <summary>
        /// Gets the size of the text (cached).
        /// </summary>
        public Vector2 TextSize => _textSize;

        /// <summary>
        /// Occurs when target view offset gets changed.
        /// </summary>
        public event Action TargetViewOffsetChanged;

        /// <summary>
        /// Gets the current view offset (text scrolling offset). Includes the smoothing.
        /// </summary>
        public Vector2 ViewOffset => _viewOffset;

        /// <summary>
        /// Gets or sets the target view offset (text scrolling offset).
        /// </summary>
        [NoAnimate, NoSerialize]
        public Vector2 TargetViewOffset
        {
            get => _targetViewOffset;
            set
            {
                value = Vector2.Round(value);
                if (Vector2.NearEqual(ref value, ref _targetViewOffset))
                    return;

                _targetViewOffset = _viewOffset = value;
                OnTargetViewOffsetChanged();
            }
        }

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
        /// Sets the text.
        /// </summary>
        /// <param name="value">The value.</param>
        protected void SetText(string value)
        {
            // Prevent from null problems
            if (value == null)
                value = string.Empty;

            // Filter text
            if (value.IndexOf('\r') != -1)
                value = value.Replace("\r", "");

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
                Vector2 caretPos = GetCharPosition(CaretPosition, out var height);
                return new Rectangle(
                                     caretPos.X - (caretWidth * 0.5f),
                                     caretPos.Y,
                                     caretWidth,
                                     height);
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
            AutoFocus = false;

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
            TargetViewOffset = Vector2.Zero;
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

            var right = SelectionRight;
            Insert(clipboardText);
            SetSelection(Mathf.Max(right, 0) + clipboardText.Length);
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
                TargetViewOffset = Vector2.Zero;
                return;
            }

            Rectangle caretBounds = CaretBounds;
            Rectangle textArea = TextRectangle;

            // Update view offset (caret needs to be in a view)
            Vector2 caretInView = caretBounds.Location - _targetViewOffset;
            Vector2 clampedCaretInView = Vector2.Clamp(caretInView, textArea.UpperLeft, textArea.BottomRight);
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
        public virtual int CharIndexAtPoint(ref Vector2 location)
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
                    _text = _text.Remove(left, selectionLength);

                _text = _text.Insert(left, str);

                SetSelection(left + 1);
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
            else if (SelectionRight < TextLength)
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
            else if (SelectionLeft > 0)
            {
                int position;
                if (ctrl)
                    position = FindPrevWordBegin();
                else
                    position = _selectionEnd - 1;

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

        private int FindLineDownChar(int index)
        {
            if (!IsMultiline)
                return 0;

            Vector2 location = GetCharPosition(index, out var height);
            location.Y += height;

            return HitTestText(location);
        }

        private int FindLineUpChar(int index)
        {
            if (!IsMultiline)
                return _text.Length;

            Vector2 location = GetCharPosition(index, out var height);
            location.Y -= height;

            return HitTestText(location);
        }

        /// <summary>
        /// Calculates total text size. Called by <see cref="OnTextChanged"/> to cache the text size.
        /// </summary>
        /// <returns>The total text size.</returns>
        public abstract Vector2 GetTextSize();

        /// <summary>
        /// Calculates character position for given character index.
        /// </summary>
        /// <param name="index">The text position to get it's coordinates.</param>
        /// <param name="height">The character height (at the given character position).</param>
        /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
        public abstract Vector2 GetCharPosition(int index, out float height);

        /// <summary>
        /// Calculates hit character index at given location.
        /// </summary>
        /// <param name="location">The location to test.</param>
        /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
        public abstract int HitTestText(Vector2 location);

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
            _viewOffset = isDeltaSlow ? _targetViewOffset : Vector2.Lerp(_viewOffset, _targetViewOffset, deltaTime * 20.0f);

            base.Update(deltaTime);
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
        public override void OnMouseMove(Vector2 location)
        {
            // Check if user is selecting
            if (_isSelecting)
            {
                // Find char index at current mouse location
                int currentIndex = CharIndexAtPoint(ref location);

                // Modify selection end
                SetSelection(_selectionStart, currentIndex);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _text.Length > 0)
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
                else
                {
                    SetSelection(hitPos);
                }

                return true;
            }

            if (button == MouseButton.Left && !IsFocused)
            {
                Focus();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                OnSelectingEnd();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Vector2 location, float delta)
        {
            if (base.OnMouseWheel(location, delta))
                return true;

            // Multiline scroll
            if (IsMultiline && _text.Length != 0)
            {
                TargetViewOffset = Vector2.Clamp(_targetViewOffset - new Vector2(0, delta * 10.0f), Vector2.Zero, new Vector2(_targetViewOffset.X, _textSize.Y));
                return true;
            }

            // No event handled
            return false;
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            Insert(c);
            return true;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            var window = Root;
            bool shiftDown = window.GetKey(KeyboardKeys.Shift);
            bool ctrDown = window.GetKey(KeyboardKeys.Control);

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
                // Restore text from start
                SetSelection(-1);
                _text = _onStartEditValue;

                Defocus();
                OnTextChanged();

                return true;
            }
            case KeyboardKeys.Return:
            {
                if (IsMultiline)
                {
                    // Insert new line
                    Insert('\n');
                }
                else
                {
                    // End editing
                    Defocus();
                }

                return true;
            }
            case KeyboardKeys.Home:
            {
                // Move caret to the first character
                SetSelection(0);
                return true;
            }
            case KeyboardKeys.End:
            {
                // Move caret after last character
                SetSelection(TextLength);
                return true;
            }
            }

            return false;
        }
    }
}
