import unittest
from ComponentErrors import ComponentErrorsEx
from Acspy.Clients.SimpleClient import PySimpleClient


class ModeTest(unittest.TestCase):
    """Test the DewarPositioner.*Mode() methods"""

    def setUp(self):
        client = PySimpleClient()
        self.dp = client.getComponent('RECEIVERS/DewarPositioner')

    def test_wrong_mode(self):
        """The set* methods must raise a ComponentsErrorEx in case of wrong mode"""
        self.assertRaises(ComponentErrorsEx, self.dp.setRewindingMode, 'FOO')

    def test_right_mode(self):
        """The argument of set* must be returned by get* in case of right mode"""
        self.dp.setRewindingMode('AUTO')
        self.assertEqual(self.dp.getRewindingMode(), 'AUTO')
        self.dp.setRewindingMode('MANUAL')


if __name__ == '__main__':
    unittest.main()
