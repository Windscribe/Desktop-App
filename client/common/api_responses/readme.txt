Here are all the wrappers over the json of the ServerAPI answers.
Are in commons because they are used by both the engine and the gui.
Some of them are serializable, as they can be saved in settings.
No json validation is required here, as the input should already be valid json(coming from wsnet).
Probably in the future they need to be transferred to the wsnet library.