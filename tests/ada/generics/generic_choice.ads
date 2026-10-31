generic
    type Element is private;
    with function "<" (Left, Right : Element) return Boolean is <>;
function Generic_Choice (Left, Right : Element) return Element;
