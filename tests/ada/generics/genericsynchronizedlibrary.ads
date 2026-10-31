with Ada.Tags;
with GenericSynchronizedModel;
generic
    type Base is limited interface and GenericSynchronizedModel.Base;
    type Extra is synchronized interface and GenericSynchronizedModel.Extra;
    type T is abstract synchronized new Base and Extra with private;
package GenericSynchronizedLibrary is
    function Identity return Ada.Tags.Tag;
    function Invoke (X : T'Class) return Integer;
end GenericSynchronizedLibrary;
