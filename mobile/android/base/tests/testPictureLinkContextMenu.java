package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

public class testPictureLinkContextMenu extends ContentContextMenuTest {

    // Test website strings
    private static String PICTURE_PAGE_URL;
    private static String BLANK_PAGE_URL;
    private static final String PICTURE_PAGE_TITLE = "Picture Link";
    private static final String photoMenuItems [] = { "Copy Image Location", "Share Image", "Set Image As", "Save Image", "Open Link in New Tab", "Open Link in Private Tab", "Copy Link", "Share Link", "Bookmark Link"};

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testPictureLinkContextMenu() {
        blockForGeckoReady();

        PICTURE_PAGE_URL=getAbsoluteUrl("/robocop/robocop_picture_link.html");
        BLANK_PAGE_URL=getAbsoluteUrl("/robocop/robocop_blank_02.html");
        inputAndLoadUrl(PICTURE_PAGE_URL);
        waitForText(PICTURE_PAGE_TITLE);

        verifyContextMenuItems(photoMenuItems);
        verifyCopyOption(photoMenuItems[0], "Firefox.jpg"); // Test the "Copy Image Location" option
        verifyShareOption(photoMenuItems[1], PICTURE_PAGE_TITLE); // Test the "Share Image" option
        openTabFromContextMenu(photoMenuItems[4],2); // Test the "Open in New Tab" option - expecting 2 tabs: the original and the new one
        openTabFromContextMenu(photoMenuItems[5],2); // Test the "Open in Private Tab" option - expecting only 2 tabs in normal mode
        verifyCopyOption(photoMenuItems[6], BLANK_PAGE_URL); // Test the "Copy Link" option
        verifyShareOption(photoMenuItems[7], PICTURE_PAGE_TITLE); // Test the "Share Link" option
        verifyBookmarkLinkOption(photoMenuItems[8],BLANK_PAGE_URL); // Test the "Bookmark Link" option
    }

    @Override
    public void tearDown() throws Exception {
        mDatabaseHelper.deleteBookmark(BLANK_PAGE_URL);
        super.tearDown();
    }
}
