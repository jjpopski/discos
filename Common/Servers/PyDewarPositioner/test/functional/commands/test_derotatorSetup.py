import unittest
import mocker
from DewarPositioner.DewarPositionerImpl import DewarPositionerImpl


class DerotatorSetupCmdTest(unittest.TestCase):
    """Test the derotatorSetup and derotatorGetActualSetup commands"""

    def test_setupcmd(self):
        dp = DewarPositionerImpl()
        success, answer = dp.command('derotatorSetup=KKG')
        self.assertEqual(success, True)
        success, answer = dp.command('derotatorGetActualSetup')
        self.assertEqual(success, True)
        self.assertRegexpMatches(answer, 'KKG')

if __name__ == '__main__':
    unittest.main()
