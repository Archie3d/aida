generic
    type Element is private;
    pragma Preelaborable_Initialization (Element);
package Categorized_Generic is
    pragma Preelaborate;
    Item : Element;
end Categorized_Generic;
